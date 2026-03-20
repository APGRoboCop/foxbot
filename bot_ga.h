//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_ga.h
//
// Genetic Algorithm for neural network training - adapted from RCBot1 by Paul Murphy 'Cheeseh'
// (http://www.rcbot.net)
//
// Original RCBot code is free software under the GNU General Public License v2+
// This adaptation maintains the same license.
//

#ifndef BOT_GA_H
#define BOT_GA_H

#include <cstdio>
#include <cstdint>
#include <vector>

typedef float ga_value;

// ==================== IIndividual ====================
// Interface for a GA individual (a candidate solution).
class IIndividual
{
public:
	virtual ~IIndividual() = default;

	ga_value getFitness() const { return m_fFitness; }
	void setFitness(const ga_value fVal) { m_fFitness = fVal; }

	virtual void crossOver(IIndividual* other) = 0;
	virtual void mutate() = 0;
	virtual void mutateWithRate(float rate) { mutate(); }
	virtual void clear() = 0;
	virtual IIndividual* copy() = 0;

protected:
	ga_value m_fFitness = 0.0f;
};

// ==================== FoxGAValues ====================
// A GA individual whose chromosome is a vector of floats (e.g. NN weights).
class FoxGAValues : public IIndividual
{
public:
	FoxGAValues();
	explicit FoxGAValues(const std::vector<ga_value>& values);

	void crossOver(IIndividual* other) override;
	void mutate() override;
	void mutateWithRate(float rate) override;
	void clear() override;
	IIndividual* copy() override;

	void setVector(const std::vector<ga_value>& values);
	void getVector(std::vector<ga_value>& values) const;

	ga_value get(unsigned iIndex) const;
	void set(unsigned iIndex, ga_value fVal);

	void add(ga_value val) { m_theValues.emplace_back(val); }
	unsigned size() const { return static_cast<unsigned>(m_theValues.size()); }

private:
	std::vector<ga_value> m_theValues;
};

// ==================== FoxPopulation ====================
class FoxGA;

class FoxPopulation
{
public:
	~FoxPopulation();

	void freeMemory();
	void setGA(FoxGA* ga) { m_ga = ga; }
	unsigned size() const { return m_theIndividuals.size(); }

	IIndividual* get(unsigned iIndex) const;
	void add(IIndividual* individual);
	void clear();

	IIndividual* getBestIndividual() const;
	ga_value totalFitness() const;
	ga_value bestFitness() const;
	ga_value averageFitness() const;
	ga_value diversity() const;

	IIndividual* pick();

private:
	std::vector<IIndividual*> m_theIndividuals;
	FoxGA* m_ga = nullptr;
};

// ==================== Selection ====================
class ISelection
{
public:
	virtual ~ISelection() = default;
	virtual IIndividual* select(FoxPopulation* population) = 0;
};

class FoxRouletteSelection : public ISelection
{
public:
	IIndividual* select(FoxPopulation* population) override;
};

class FoxTournamentSelection : public ISelection
{
public:
	explicit FoxTournamentSelection(const int tournamentSize = 3)
		: m_iTournamentSize(tournamentSize) {}

	IIndividual* select(FoxPopulation* population) override;

private:
	int m_iTournamentSize;
};

// ==================== FoxGA ====================
class FoxGA
{
public:
	explicit FoxGA(int iMaxPopSize = 0);
	FoxGA(ISelection* selectFunction, int iMaxPopSize = 0);
	~FoxGA();

	void freeLocalMemory();

	// Run one generation: selection, crossover, mutation.
	void epoch();

	void addToPopulation(IIndividual* individual);

	bool canPick() const;
	IIndividual* pick();

	void setSize(const int iSize) { m_iMaxPopSize = static_cast<unsigned>(iSize); }

	// Adaptive mutation support.
	void setAdaptiveMutation(const bool enable) { m_bUseAdaptiveMutation = enable; }
	float getAdaptiveMutateRate() const;
	unsigned getNumGenerations() const { return m_iNumGenerations; }

	bool save(std::FILE* bfp) const;
	bool load(std::FILE* bfp, int chromosize);

	static constexpr int g_iDefaultMaxPopSize = 16;
	static constexpr float g_fCrossOverRate = 0.7f;
	static constexpr float g_fMutateRate = 0.1f;
	static constexpr float g_fMaxPerturbation = 0.3f;

	unsigned m_iMaxPopSize;

private:
	FoxPopulation m_thePopulation;
	FoxPopulation m_theNewPopulation;
	unsigned m_iNumGenerations;
	float m_fPrevAvgFitness;
	bool m_bUseAdaptiveMutation;
	ISelection* m_theSelectFunction;
	IIndividual* m_bestIndividual;
};

// ==================== Combat Fitness ====================
// Tracks combat performance metrics for GA fitness evaluation.
class FoxCombatFitness
{
public:
	float damageDealt = 0.0f;
	float damageTaken = 0.0f;
	int kills = 0;
	int deaths = 0;
	float survivalTime = 0.0f;

	FoxCombatFitness() = default;

	void reset()
	{
		damageDealt = 0.0f;
		damageTaken = 0.0f;
		kills = 0;
		deaths = 0;
		survivalTime = 0.0f;
	}

	void recordHit(const float damage) { damageDealt += damage; }
	void recordDamageTaken(const float damage) { damageTaken += damage; }
	void recordKill() { kills++; }
	void recordDeath() { deaths++; }
	void updateSurvivalTime(const float deltaTime) { survivalTime += deltaTime; }

	ga_value calculateFitness() const;
};

// ==================== GA-Trained NN ====================
// Combines the FoxNN with a GA for weight optimisation.
class FoxNN;

class FoxNNGATrained
{
public:
	FoxNNGATrained();
	~FoxNNGATrained();

	// Call after each combat encounter finishes.
	// Provides fitness and feeds the GA; picks new weights from the evolved population.
	void submitFitness(ga_value fitness);

	// Apply the current individual's weights to the given NN.
	void applyWeights(FoxNN& nn) const;

	// Extract weights from a NN and seed them as the current individual.
	void seedFromNN(const FoxNN& nn);

	bool save(std::FILE* bfp) const;
	bool load(std::FILE* bfp);

	FoxGA& getGA() { return m_ga; }

private:
	FoxGA m_ga;
	FoxGAValues* m_pCurrentIndividual;
};

// ==================== Global GA instance for combat NN ====================
extern FoxNNGATrained g_CombatGA;

// Initialise the GA (call once at map load, after the NN is initialised).
void FoxCombatGAInit();

// Save GA state to disk (uses current map name).
bool FoxCombatGASave();

// Save GA state to disk for a specific map name.
bool FoxCombatGASaveForMap(const char* mapname);

#endif // BOT_GA_H
