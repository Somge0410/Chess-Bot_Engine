#pragma once 
#include "eval_params.h"
#include "eval_internal.h"
struct Trace {
	int counts[PARAM_COUNT] = { 0 };
	void add(EvalParam param, int count = 1) {
		counts[param] += count;
	}
};
static int get_trace_eval(Trace* trace, const EvaluationResult weights[PARAM_COUNT], int game_phase) {
	EvaluationResult eval = { 0,0 };
	for (int i = 0; i < PARAM_COUNT; i++) {
		eval += weights[i] * trace->counts[i];
	}
	return tapered(eval, game_phase);
}
template <bool isTracing>
void addTerm(EvaluationResult& score, EvalParam param, int count = 1, Trace* trace = nullptr) {
	score += EvalWeights[param] * count;
	if constexpr (isTracing) {
		if (trace) trace->add(param, count);
	}
}