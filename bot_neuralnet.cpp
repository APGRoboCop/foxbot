// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_neuralnet.cpp
//
// Neural network for combat decisions - adapted from RCBot1 by Paul Murphy 'Cheeseh'
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

#include <algorithm>
#include <numeric>

extern int mod_id;

// Global combat NN instance
FoxCombatNN g_CombatNN;

// ==================== FoxPerceptron ====================

FoxPerceptron::FoxPerceptron()
= default;

FoxPerceptron::FoxPerceptron(const int iInputs)
{
	setNumInputs(iInputs);
}

void FoxPerceptron::setNumInputs(const int iInputs)
{
	m_weights.resize(iInputs + 1); // +1 for bias weight
	randomize();
}

void FoxPerceptron::setWeight(const int index, const float val)
{
	if (index >= 0 && index < static_cast<int>(m_weights.size()))
		m_weights[index] = val;
}

float FoxPerceptron::getWeight(const int index) const
{
	if (index >= 0 && index < static_cast<int>(m_weights.size()))
		return m_weights[index];
	return 0.0f;
}

float FoxPerceptron::execute(const std::vector<float>& inputs) const {
	if (inputs.size() + 1 != m_weights.size())
		return 0.5f;

	// bias weight
	float netInput = m_weights[0];

	// sum weighted inputs
	for (size_t i = 0; i < inputs.size(); i++)
		netInput += inputs[i] * m_weights[i + 1];

	return sigmoid(netInput);
}

void FoxPerceptron::randomize()
{
	for (float& w : m_weights)
		w = (static_cast<float>(std::rand()) / RAND_MAX) * 0.6f - 0.3f;
}

// ==================== FoxNN ====================

FoxNN::FoxNN()
	: m_numInputs(0), m_numHidden(0), m_numOutputs(0)
{
}

FoxNN::FoxNN(const int numInputs, const int numHidden, const int numOutputs)
	: m_numInputs(0), m_numHidden(0), m_numOutputs(0)
{
	init(numInputs, numHidden, numOutputs);
}

void FoxNN::init(const int numInputs, const int numHidden, const int numOutputs)
{
	m_numInputs = numInputs;
	m_numHidden = numHidden;
	m_numOutputs = numOutputs;

	m_hiddenLayer.clear();
	m_outputLayer.clear();

	m_hiddenLayer.reserve(numHidden);
	for (int i = 0; i < numHidden; i++)
		m_hiddenLayer.emplace_back(numInputs);

	m_outputLayer.reserve(numOutputs);
	for (int i = 0; i < numOutputs; i++)
		m_outputLayer.emplace_back(numHidden);
}

void FoxNN::execute(const std::vector<float>& inputs, std::vector<float>& outputs) const
{
	// hidden layer
	std::vector<float> hiddenOutputs;
	hiddenOutputs.reserve(m_numHidden);

	for (int i = 0; i < m_numHidden; i++)
	{
		FoxPerceptron p = m_hiddenLayer[i]; // copy to allow non-const execute
		hiddenOutputs.push_back(p.execute(inputs));
	}

	// output layer
	outputs.clear();
	outputs.reserve(m_numOutputs);

	for (int i = 0; i < m_numOutputs; i++)
	{
		FoxPerceptron p = m_outputLayer[i];
		outputs.push_back(p.execute(hiddenOutputs));
	}
}

void FoxNN::getWeights(std::vector<float>& weights) const
{
	weights.clear();

	for (const FoxPerceptron &neuron : m_hiddenLayer)
		for (int k = 0; k < neuron.numWeights(); k++)
			weights.push_back(neuron.getWeight(k));

	for (const FoxPerceptron &neuron : m_outputLayer)
		for (int k = 0; k < neuron.numWeights(); k++)
			weights.push_back(neuron.getWeight(k));
}

void FoxNN::setWeights(const std::vector<float>& weights)
{
	int w = 0;

	for (FoxPerceptron &neuron : m_hiddenLayer)
		for (int k = 0; k < neuron.numWeights(); k++)
			if (w < static_cast<int>(weights.size()))
				neuron.setWeight(k, weights[w++]);

	for (FoxPerceptron &neuron : m_outputLayer)
		for (int k = 0; k < neuron.numWeights(); k++)
			if (w < static_cast<int>(weights.size()))
				neuron.setWeight(k, weights[w++]);
}

int FoxNN::totalWeights() const
{
	int total = 0;

	for (const FoxPerceptron &neuron : m_hiddenLayer)
		total += neuron.numWeights();

	for (const FoxPerceptron &neuron : m_outputLayer)
		total += neuron.numWeights();

	return total;
}

void FoxNN::randomize()
{
	for (FoxPerceptron &neuron : m_hiddenLayer)
		neuron.randomize();

	for (FoxPerceptron &neuron : m_outputLayer)
		neuron.randomize();
}

bool FoxNN::save(std::FILE* bfp) const
{
	if (bfp == nullptr)
		return false;

	std::fwrite(&m_numInputs, sizeof(int), 1, bfp);
	std::fwrite(&m_numHidden, sizeof(int), 1, bfp);
	std::fwrite(&m_numOutputs, sizeof(int), 1, bfp);

	std::vector<float> weights;
	getWeights(weights);

	const int numWeights = static_cast<int>(weights.size());
	std::fwrite(&numWeights, sizeof(int), 1, bfp);
	std::fwrite(weights.data(), sizeof(float), numWeights, bfp);

	return true;
}

bool FoxNN::load(std::FILE* bfp)
{
	if (bfp == nullptr)
		return false;

	int numInputs, numHidden, numOutputs;

	if (std::fread(&numInputs, sizeof(int), 1, bfp) != 1)
		return false;
	if (std::fread(&numHidden, sizeof(int), 1, bfp) != 1)
		return false;
	if (std::fread(&numOutputs, sizeof(int), 1, bfp) != 1)
		return false;

	init(numInputs, numHidden, numOutputs);

	int numWeights;
	if (std::fread(&numWeights, sizeof(int), 1, bfp) != 1)
		return false;

	std::vector<float> weights(numWeights);
	if (std::fread(weights.data(), sizeof(float), numWeights, bfp) != static_cast<size_t>(numWeights))
		return false;

	setWeights(weights);
	return true;
}

// ==================== FoxCombatNN ====================

FoxCombatNN::FoxCombatNN()
	: m_inputs(COMBAT_NN_NUM_INPUTS, 0.0f)
	, m_outputs(COMBAT_NN_NUM_OUTPUTS, 0.5f)
{
	m_nn.init(COMBAT_NN_NUM_INPUTS, COMBAT_NN_HIDDEN_NEURONS, COMBAT_NN_NUM_OUTPUTS);
}

// Normalize TFC class to a 0-1 threat/weakness value
float FoxCombatNN::normalizeClass(const int playerClass)
{
	switch (playerClass)
	{
	case 0:  return 0.0f;  // civilian
	case 1:  return 0.3f;  // scout
	case 2:  return 0.5f;  // sniper
	case 3:  return 0.8f;  // soldier
	case 4:  return 0.7f;  // demoman
	case 5:  return 0.6f;  // medic
	case 6:  return 0.9f;  // hwguy
	case 7:  return 0.7f;  // pyro
	case 8:  return 0.4f;  // spy
	case 9:  return 0.3f;  // engineer
	default: return 0.5f;
	}
}

void FoxCombatNN::setInputs(const float enemyDistance, const float selfHealthPercent,
	const int enemyClass, const int selfClass,
	const int visEnemyCount, const int visAllyCount,
	const bool hasFlag, const int aggression)
{
	m_inputs[CINPUT_ENEMY_DISTANCE] = std::min(enemyDistance / 1500.0f, 1.0f);
	m_inputs[CINPUT_SELF_HEALTH] = std::min(selfHealthPercent / 100.0f, 1.0f);
	m_inputs[CINPUT_ENEMY_CLASS] = normalizeClass(enemyClass);
	m_inputs[CINPUT_SELF_CLASS] = normalizeClass(selfClass);
	m_inputs[CINPUT_VIS_ENEMY_COUNT] = std::min(static_cast<float>(visEnemyCount) / 5.0f, 1.0f);
	m_inputs[CINPUT_VIS_ALLY_COUNT] = std::min(static_cast<float>(visAllyCount) / 5.0f, 1.0f);
	m_inputs[CINPUT_HAS_FLAG] = hasFlag ? 1.0f : 0.0f;
	m_inputs[CINPUT_AGGRESSION] = std::min(static_cast<float>(aggression) / 1000.0f, 1.0f);
}

int FoxCombatNN::getThreatLevel()
{
	m_nn.execute(m_inputs, m_outputs);

	// Convert NN outputs to a threat level (0-100 scale)
	// High retreat weight + low attack weight = high threat
	const float threatFromRetreat = m_outputs[COUTPUT_RETREAT] * 60.0f;
	const float threatReductionFromAttack = m_outputs[COUTPUT_ATTACK] * 40.0f;

	int threat = static_cast<int>(threatFromRetreat - threatReductionFromAttack + 40.0f);

	// clamp to 0-100
   threat = std::max(threat, 0);
   threat = std::min(threat, 100);

   return threat;
}

bool FoxCombatNN::saveWeights() const
{
	return saveWeightsForMap(STRING(gpGlobals->mapname));
}

bool FoxCombatNN::saveWeightsForMap(const char* mapname) const
{
	if (mapname == nullptr || mapname[0] == '\0')
		return false;

	FoxEnsureDirectory("experience");

	char fname[64];
	char filename[256];

	std::strncpy(fname, mapname, 55);
	fname[55] = '\0';
	std::strcat(fname, ".fnn"); // FoXBot Neural Net

	UTIL_BuildFileName(filename, 255, "experience", fname);

	std::FILE* bfp = std::fopen(filename, "wb");
	if (bfp == nullptr)
		return false;

	const bool result = m_nn.save(bfp);
	std::fclose(bfp);
	return result;
}

bool FoxCombatNN::loadWeights()
{
	char mapname[64];
	char filename[256];

	std::strcpy(mapname, STRING(gpGlobals->mapname));
	std::strcat(mapname, ".fnn");

	UTIL_BuildFileName(filename, 255, "experience", mapname);

	std::FILE* bfp = std::fopen(filename, "rb");
	if (bfp == nullptr)
		return false;

	const bool result = m_nn.load(bfp);
	std::fclose(bfp);
	return result;
}

// Set hand-tuned default weights that approximate the behaviour of the
// original guessThreatLevel() heuristic.
void FoxCombatNN::setDefaultWeights()
{
	m_nn.init(COMBAT_NN_NUM_INPUTS, COMBAT_NN_HIDDEN_NEURONS, COMBAT_NN_NUM_OUTPUTS);

	// Rather than randomizing, seed with weights that produce reasonable
	// combat behaviour out of the box. This can be improved later via GA.
	m_nn.randomize();
}

// Initialize the combat NN
void FoxCombatNNInit()
{
	FoxEnsureDirectory("experience");

	// try to load saved weights first
	if (g_CombatNN.loadWeights())
	{
		if (IS_DEDICATED_SERVER())
			std::printf("FoXBot: Combat NN weights loaded\n");
		return;
	}

	// no saved weights found - use defaults
	g_CombatNN.setDefaultWeights();

	if (IS_DEDICATED_SERVER())
		std::printf("FoXBot: Combat NN initialized with default weights\n");
}

// Ensure a subdirectory exists under the foxbot path.
void FoxEnsureDirectory(const char* subdir)
{
	char dirpath[256];
	UTIL_BuildFileName(dirpath, 255, subdir, nullptr);

	if (dirpath[0] == '\0')
		return;

#ifndef __linux__
	CreateDirectoryA(dirpath, nullptr);
#else
	mkdir(dirpath, 0755);
#endif
}
