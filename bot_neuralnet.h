//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_neuralnet.h
//
// Neural network for combat decisions - adapted from RCBot1 by Paul Murphy 'Cheeseh'
// (http://www.rcbot.net)
//
// Original RCBot code is free software under the GNU General Public License v2+
// This adaptation maintains the same license.
//

#ifndef BOT_NEURALNET_H
#define BOT_NEURALNET_H

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

constexpr int COMBAT_NN_NUM_INPUTS = 8;
constexpr int COMBAT_NN_NUM_OUTPUTS = 4;
constexpr int COMBAT_NN_HIDDEN_NEURONS = 6;

enum eCombatNNInput : std::uint8_t {
    CINPUT_ENEMY_DISTANCE = 0,
    CINPUT_SELF_HEALTH = 1,
    CINPUT_ENEMY_CLASS = 2,
    CINPUT_SELF_CLASS = 3,
    CINPUT_VIS_ENEMY_COUNT = 4,
    CINPUT_VIS_ALLY_COUNT = 5,
    CINPUT_HAS_FLAG = 6,
    CINPUT_AGGRESSION = 7
};

enum eCombatNNOutput : std::uint8_t {
    COUTPUT_ATTACK = 0,
    COUTPUT_RETREAT = 1,
    COUTPUT_STRAFE = 2,
    COUTPUT_JUMP = 3
};

class FoxPerceptron
{
public:
    FoxPerceptron();
    explicit FoxPerceptron(int iInputs);
    void setNumInputs(int iInputs);
    void setWeight(int index, float val);
    float getWeight(int index) const;
    int numWeights() const { return static_cast<int>(m_weights.size()); }
    float execute(const std::vector<float>& inputs) const;
    void randomize();
private:
    static float sigmoid(const float x) { return 1.0f / (1.0f + std::exp(-x)); }
    std::vector<float> m_weights;
};

class FoxNN
{
public:
    FoxNN();
    FoxNN(int numInputs, int numHidden, int numOutputs);
    void init(int numInputs, int numHidden, int numOutputs);
    void execute(const std::vector<float>& inputs, std::vector<float>& outputs) const;
    void getWeights(std::vector<float>& weights) const;
    void setWeights(const std::vector<float>& weights);
    int totalWeights() const;
    void randomize();
    bool save(std::FILE* bfp) const;
    bool load(std::FILE* bfp);
private:
    std::vector<FoxPerceptron> m_hiddenLayer;
    std::vector<FoxPerceptron> m_outputLayer;
    int m_numInputs;
    int m_numHidden;
    int m_numOutputs;
};

class FoxCombatNN
{
public:
    FoxCombatNN();
    void setInputs(float enemyDistance, float selfHealthPercent,
        int enemyClass, int selfClass,
        int visEnemyCount, int visAllyCount,
        bool hasFlag, int aggression);
    int getThreatLevel();
    float getAttackWeight() const { return m_outputs[COUTPUT_ATTACK]; }
    float getRetreatWeight() const { return m_outputs[COUTPUT_RETREAT]; }
    float getStrafeWeight() const { return m_outputs[COUTPUT_STRAFE]; }
    float getJumpWeight() const { return m_outputs[COUTPUT_JUMP]; }
    FoxNN& getNN() { return m_nn; }
    const FoxNN& getNN() const { return m_nn; }
    bool saveWeights() const;
    bool saveWeightsForMap(const char* mapname) const;
    bool loadWeights();
    void setDefaultWeights();
private:
    static float normalizeClass(int playerClass);
    FoxNN m_nn;
    std::vector<float> m_inputs;
    std::vector<float> m_outputs;
};

extern FoxCombatNN g_CombatNN;
void FoxCombatNNInit();

// Ensure a subdirectory exists under the foxbot path.
void FoxEnsureDirectory(const char* subdir);

#endif // BOT_NEURALNET_H