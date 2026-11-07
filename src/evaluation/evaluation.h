#pragma once
#include <algorithm>
#include "bitboard.h"
#include "position.h"
#include "eval_params.h"
#include "trace.h"


template <bool isTracing = false>
int evaluate(const Position& pos, Trace* trace = nullptr, uint8_t terms_mask = EvalAll,
    PawnTable* pawn_table = nullptr);

bool trace_eval_agree(const Position& pos, const EvaluationResult weights[PARAM_COUNT]);

template <bool isTracing>
void eval_material(EvaluationResult& score, const Position& pos, Trace* trace);

template <bool isTracing>
void eval_positional(EvaluationResult& score, const Position& pos, Trace* trace);




