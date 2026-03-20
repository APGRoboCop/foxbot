// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_ga.cpp
//
// Genetic Algorithm for neural network training - adapted from RCBot1 by Paul Murphy 'Cheeseh'
// (http://www.rcbot.net)
//
// Original RCBot code is free software under the GNU General Public License v2+
// This adaptation maintains the same license.
//

#include "extdll.h"
#include "util.h"
#include <enginecallback.h>

#include "bot.h"
#include "waypoint.h"
#include "bot_neuralnet.h"
#include "bot_ga.h"

#include <algorithm>
#include <cmath>
#include <numeric>

extern int mod_id;

// Global GA instance for combat NN
FoxNNGATrained g_CombatGA;

// ==================== FoxGAValues ====================

FoxGAValues::FoxGAValues()
{
   FoxGAValues::clear();
	setFitness(0);
}

FoxGAValues::FoxGAValues(const std::vector<ga_value>& values)
{
   FoxGAValues::clear();
	setFitness(0);
	setVector(values);
}

void FoxGAValues::clear()
{
	m_theValues.clear();
}

void FoxGAValues::crossOver(IIndividual* other)
{
	if (m_theValues.empty())
		return;

	const unsigned iPoint = RANDOM_LONG(0, static_cast<int>(m_theValues.size()) - 1);
	FoxGAValues* vother = static_cast<FoxGAValues*>(other);

	for (unsigned i = 0; i < iPoint; i++)
		std::swap(m_theValues[i], vother->m_theValues[i]);
}

void FoxGAValues::mutate()
{
	for (unsigned i = 0; i < m_theValues.size(); i++)
	{
		if (RANDOM_FLOAT(0, 1) < FoxGA::g_fMutateRate)
		{
			const float fCurrentVal = get(i);
			set(i, fCurrentVal + fCurrentVal * (-1.0f + RANDOM_FLOAT(0, 2)) * FoxGA::g_fMaxPerturbation);
		}
	}
}

void FoxGAValues::mutateWithRate(const float rate)
{
	for (unsigned i = 0; i < m_theValues.size(); i++)
	{
		if (RANDOM_FLOAT(0, 1) < rate)
		{
			const float fCurrentVal = get(i);
			const float perturbScale = rate / FoxGA::g_fMutateRate;
			const float perturbation = FoxGA::g_fMaxPerturbation * perturbScale;
			set(i, fCurrentVal + fCurrentVal * (-1.0f + RANDOM_FLOAT(0, 2)) * perturbation);
		}
	}
}

IIndividual* FoxGAValues::copy()
{
	IIndividual* individual = new FoxGAValues(m_theValues);
	individual->setFitness(getFitness());
	return individual;
}

void FoxGAValues::setVector(const std::vector<ga_value>& values)
{
	m_theValues = values;
}

void FoxGAValues::getVector(std::vector<ga_value>& values) const
{
	values = m_theValues;
}

ga_value FoxGAValues::get(const unsigned iIndex) const
{
	if (iIndex < m_theValues.size())
		return m_theValues[iIndex];
	return 0.0f;
}

void FoxGAValues::set(const unsigned iIndex, const ga_value fVal)
{
	if (iIndex < m_theValues.size())
		m_theValues[iIndex] = fVal;
}

// ==================== FoxPopulation ====================

FoxPopulation::~FoxPopulation()
{
	freeMemory();
}

void FoxPopulation::freeMemory()
{
	for (const IIndividual* individual : m_theIndividuals)
		delete individual;

	m_theIndividuals.clear();
}

IIndividual* FoxPopulation::get(const unsigned iIndex) const
{
	if (iIndex < m_theIndividuals.size())
		return m_theIndividuals[iIndex];
	return nullptr;
}

void FoxPopulation::add(IIndividual* individual)
{
	m_theIndividuals.emplace_back(individual);
}

void FoxPopulation::clear()
{
	freeMemory();
}

IIndividual* FoxPopulation::getBestIndividual() const
{
	if (m_theIndividuals.empty())
		return nullptr;

   const std::vector<IIndividual *>::const_iterator best = std::max_element(
      m_theIndividuals.cbegin(), m_theIndividuals.cend(),
      [](const IIndividual *a, const IIndividual *b) {
         return a->getFitness() < b->getFitness();
      });

	return *best;
}

ga_value FoxPopulation::totalFitness() const
{
	return std::accumulate(m_theIndividuals.cbegin(), m_theIndividuals.cend(), 0.0f,
		[](const ga_value sum, const IIndividual* individual) {
			return sum + individual->getFitness();
		});
}

ga_value FoxPopulation::bestFitness() const
{
	if (m_theIndividuals.empty())
		return 0.0f;

   const std::vector<IIndividual*>::const_iterator best = std::max_element(
      m_theIndividuals.cbegin(), m_theIndividuals.cend(),
      [](const IIndividual* a, const IIndividual* b)
      {
         return a->getFitness() < b->getFitness();
      });

   return (*best)->getFitness();
}

ga_value FoxPopulation::averageFitness() const
{
	if (m_theIndividuals.empty())
		return 0.0f;
	return totalFitness() / static_cast<ga_value>(m_theIndividuals.size());
}

ga_value FoxPopulation::diversity() const
{
	if (m_theIndividuals.size() < 2)
		return 0.0f;

	const ga_value avgFit = averageFitness();
	const ga_value bestFit = bestFitness();

	if (bestFit < 0.0001f)
		return 1.0f;

	ga_value variance = 0.0f;
	for (const IIndividual* individual : m_theIndividuals)
	{
		const ga_value diff = individual->getFitness() - avgFit;
		variance += diff * diff;
	}
	variance /= static_cast<ga_value>(m_theIndividuals.size());

	const ga_value stdDev = std::sqrt(variance);
	return std::min(stdDev / bestFit, 1.0f);
}

IIndividual* FoxPopulation::pick()
{
	if (m_theIndividuals.empty())
		return nullptr;

	IIndividual* to_return = m_theIndividuals.back();
	m_theIndividuals.pop_back();
	return to_return;
}

// ==================== Selection ====================

IIndividual* FoxRouletteSelection::select(FoxPopulation* population)
{
	const ga_value fFitnessSlice = RANDOM_FLOAT(0, population->totalFitness());
	ga_value fFitnessSoFar = 0.0f;

	for (unsigned i = 0; i < population->size(); i++)
	{
		IIndividual* individual = population->get(i);
		fFitnessSoFar += individual->getFitness();

		if (fFitnessSoFar >= fFitnessSlice)
			return individual;
	}

	return population->get(0);
}

IIndividual* FoxTournamentSelection::select(FoxPopulation* population)
{
	IIndividual* best = nullptr;

	for (int i = 0; i < m_iTournamentSize; i++)
	{
		const unsigned idx = RANDOM_LONG(0, static_cast<int>(population->size()) - 1);
		IIndividual* candidate = population->get(idx);

		if (!best || candidate->getFitness() > best->getFitness())
			best = candidate;
	}

	return best;
}

// ==================== FoxGA ====================

FoxGA::FoxGA(const int iMaxPopSize)
	: m_iMaxPopSize(0), m_iNumGenerations(0), m_fPrevAvgFitness(0.0f),
	  m_bUseAdaptiveMutation(true), m_theSelectFunction(new FoxRouletteSelection()),
	  m_bestIndividual(nullptr)
{
	m_thePopulation.setGA(this);
	m_theNewPopulation.setGA(this);

	m_iMaxPopSize = iMaxPopSize > 0 ? static_cast<unsigned>(iMaxPopSize) : g_iDefaultMaxPopSize;
}

FoxGA::FoxGA(ISelection* selectFunction, const int iMaxPopSize)
	: m_iMaxPopSize(0), m_iNumGenerations(0), m_fPrevAvgFitness(0.0f),
	  m_bUseAdaptiveMutation(true), m_theSelectFunction(selectFunction),
	  m_bestIndividual(nullptr)
{
	m_thePopulation.setGA(this);
	m_theNewPopulation.setGA(this);

	m_iMaxPopSize = iMaxPopSize > 0 ? static_cast<unsigned>(iMaxPopSize) : g_iDefaultMaxPopSize;
}

FoxGA::~FoxGA()
{
	freeLocalMemory();
	delete m_theSelectFunction;
	m_theSelectFunction = nullptr;
}

void FoxGA::freeLocalMemory()
{
	m_thePopulation.freeMemory();
	m_theNewPopulation.freeMemory();
	m_iNumGenerations = 0;
	delete m_bestIndividual;
	m_bestIndividual = nullptr;
}

void FoxGA::addToPopulation(IIndividual* individual)
{
	m_thePopulation.add(individual);

	if (m_thePopulation.size() >= m_iMaxPopSize)
	{
		epoch();

		IIndividual* best = m_thePopulation.getBestIndividual();

		if (best && (!m_bestIndividual || m_bestIndividual->getFitness() < best->getFitness()))
		{
			if (m_bestIndividual && m_bestIndividual != best)
			{
				delete m_bestIndividual;
				m_bestIndividual = nullptr;
			}

			m_bestIndividual = best->copy();
		}

		m_thePopulation.freeMemory();

		if (m_bestIndividual)
			m_thePopulation.add(m_bestIndividual->copy());
	}
}

void FoxGA::epoch()
{
	m_theNewPopulation.freeMemory();

	// Elitism: preserve the best individuals unchanged
	constexpr int ELITE_COUNT = 2;
	for (int i = 0; i < ELITE_COUNT && i < static_cast<int>(m_thePopulation.size()); i++)
	{
      if (IIndividual* best = m_thePopulation.getBestIndividual())
			m_theNewPopulation.add(best->copy());
	}

	const float currentMutateRate = getAdaptiveMutateRate();

	while (m_theNewPopulation.size() < m_iMaxPopSize)
	{
		IIndividual* mum = m_theSelectFunction->select(&m_thePopulation);
		IIndividual* dad = m_theSelectFunction->select(&m_thePopulation);

		IIndividual* baby1 = mum->copy();
		IIndividual* baby2 = dad->copy();

		if (RANDOM_FLOAT(0, 1) < g_fCrossOverRate)
			baby1->crossOver(baby2);

		baby1->mutateWithRate(currentMutateRate);
		baby2->mutateWithRate(currentMutateRate);

		m_theNewPopulation.add(baby1);
		m_theNewPopulation.add(baby2);
	}

	m_fPrevAvgFitness = m_thePopulation.averageFitness();
	m_iNumGenerations++;
}

bool FoxGA::canPick() const
{
	return m_theNewPopulation.size() > 0;
}

IIndividual* FoxGA::pick()
{
	return m_theNewPopulation.pick();
}

float FoxGA::getAdaptiveMutateRate() const
{
	if (!m_bUseAdaptiveMutation)
		return g_fMutateRate;

	const ga_value bestFit = m_thePopulation.bestFitness();
	if (bestFit <= 0.0f)
		return g_fMutateRate;

	const ga_value div = m_thePopulation.diversity();

	constexpr float MIN_MUTATION = 0.05f;
	constexpr float MAX_MUTATION = 0.30f;

	const float mutationNeed = 1.0f - div;
	float adaptiveRate = MIN_MUTATION + (MAX_MUTATION - MIN_MUTATION) * mutationNeed;

	if (m_iNumGenerations < 10)
		adaptiveRate = std::max(adaptiveRate, 0.15f);

	return adaptiveRate;
}

bool FoxGA::save(std::FILE* bfp) const
{
	if (!bfp)
		return false;

	std::fwrite(&m_iMaxPopSize, sizeof(unsigned), 1, bfp);
	std::fwrite(&m_iNumGenerations, sizeof(unsigned), 1, bfp);
	return true;
}

bool FoxGA::load(std::FILE* bfp, const int chromosize)
{
	if (!bfp)
		return false;

	if (std::fread(&m_iMaxPopSize, sizeof(unsigned), 1, bfp) != 1)
		return false;
	if (std::fread(&m_iNumGenerations, sizeof(unsigned), 1, bfp) != 1)
		return false;

	return true;
}

// ==================== FoxCombatFitness ====================

ga_value FoxCombatFitness::calculateFitness() const
{
	// Damage ratio: reward dealing more damage than taking
	const ga_value damageRatio = (damageTaken > 0.0f)
		? std::min(damageDealt / damageTaken, 10.0f) / 10.0f
		: (damageDealt > 0.0f ? 1.0f : 0.0f);

	// K/D ratio
	const ga_value kdRatio = (deaths > 0)
		? std::min(static_cast<float>(kills) / static_cast<float>(deaths), 5.0f) / 5.0f
		: (kills > 0 ? 1.0f : 0.0f);

	// Survival time (normalised to 5 minutes max)
	const ga_value survivalScore = std::min(survivalTime / 300.0f, 1.0f);

	// Weighted combination
	return damageRatio * 0.35f + kdRatio * 0.40f + survivalScore * 0.25f;
}

// ==================== FoxNNGATrained ====================

FoxNNGATrained::FoxNNGATrained()
	: m_ga(16), m_pCurrentIndividual(nullptr)
{
}

FoxNNGATrained::~FoxNNGATrained()
{
	delete m_pCurrentIndividual;
	m_pCurrentIndividual = nullptr;
}

void FoxNNGATrained::submitFitness(const ga_value fitness)
{
	if (!m_pCurrentIndividual)
		return;

	m_pCurrentIndividual->setFitness(fitness);
	m_ga.addToPopulation(m_pCurrentIndividual->copy());

	// If a new generation was produced, pick a new individual
	// and apply its weights to the combat NN
	if (m_ga.canPick())
	{
		delete m_pCurrentIndividual;
		m_pCurrentIndividual = static_cast<FoxGAValues*>(m_ga.pick());

		applyWeights(g_CombatNN.getNN());
	}
}

void FoxNNGATrained::applyWeights(FoxNN& nn) const
{
	if (!m_pCurrentIndividual)
		return;

	std::vector<ga_value> weights;
	m_pCurrentIndividual->getVector(weights);
	nn.setWeights(weights);
}

void FoxNNGATrained::seedFromNN(const FoxNN& nn)
{
	std::vector<ga_value> weights;
	nn.getWeights(weights);

	delete m_pCurrentIndividual;
	m_pCurrentIndividual = new FoxGAValues(weights);
}

bool FoxNNGATrained::save(std::FILE* bfp) const
{
	if (!bfp)
		return false;

	if (!m_ga.save(bfp))
		return false;

	// save current individual weights
	if (m_pCurrentIndividual)
	{
		std::vector<ga_value> weights;
		m_pCurrentIndividual->getVector(weights);
		const int numWeights = static_cast<int>(weights.size());
		std::fwrite(&numWeights, sizeof(int), 1, bfp);
		if (numWeights > 0)
			std::fwrite(weights.data(), sizeof(ga_value), numWeights, bfp);
	}
	else
	{
      constexpr int zero = 0;
		std::fwrite(&zero, sizeof(int), 1, bfp);
	}

	return true;
}

bool FoxNNGATrained::load(std::FILE* bfp)
{
	if (!bfp)
		return false;

	const int chromosize = g_CombatNN.getNN().totalWeights();
	if (!m_ga.load(bfp, chromosize))
		return false;

	int numWeights = 0;
	if (std::fread(&numWeights, sizeof(int), 1, bfp) != 1)
		return false;

	if (numWeights > 0)
	{
		std::vector<ga_value> weights(numWeights);
		if (std::fread(weights.data(), sizeof(ga_value), numWeights, bfp) != static_cast<size_t>(numWeights))
			return false;

		delete m_pCurrentIndividual;
		m_pCurrentIndividual = new FoxGAValues(weights);
	}

	return true;
}

// ==================== Global functions ====================

void FoxCombatGAInit()
{
	char mapname[64];
	char filename[256];

	std::strcpy(mapname, STRING(gpGlobals->mapname));
	std::strcat(mapname, ".fga"); // FoXBot Genetic Algorithm

	UTIL_BuildFileName(filename, 255, "experience", mapname);

	std::FILE* bfp = std::fopen(filename, "rb");
	if (bfp != nullptr)
	{
		const bool result = g_CombatGA.load(bfp);
		std::fclose(bfp);

		if (result)
		{
			// apply loaded weights to the NN
			g_CombatGA.applyWeights(g_CombatNN.getNN());

			if (IS_DEDICATED_SERVER())
				std::printf("FoXBot: Combat GA state loaded from %s\n", filename);
			return;
		}
	}

	// no saved GA state - seed from the current NN weights
	g_CombatGA.seedFromNN(g_CombatNN.getNN());

	if (IS_DEDICATED_SERVER())
		std::printf("FoXBot: Combat GA initialized (seeded from NN)\n");
}

bool FoxCombatGASave()
{
	return FoxCombatGASaveForMap(STRING(gpGlobals->mapname));
}

bool FoxCombatGASaveForMap(const char* mapname)
{
	if (mapname == nullptr || mapname[0] == '\0')
		return false;

	FoxEnsureDirectory("experience");

	char fname[64];
	char filename[256];

	std::strncpy(fname, mapname, 55);
	fname[55] = '\0';
	std::strcat(fname, ".fga");

	UTIL_BuildFileName(filename, 255, "experience", fname);

	std::FILE* bfp = std::fopen(filename, "wb");
	if (bfp == nullptr)
		return false;

	const bool result = g_CombatGA.save(bfp);
	std::fclose(bfp);
	return result;
}
