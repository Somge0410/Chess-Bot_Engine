# Chess-Bot

A UCI-compatible chess engine written in C++20, featuring multi-threaded search with iterative deepening, aspiration windows, null-move pruning, futility pruning, late move reductions, and a transposition table.

## Features

- **UCI Protocol** – works with any UCI-compatible GUI (Arena, CuteChess, etc.)
- **Multi-threaded search** – automatically uses all available CPU cores
- **Transposition table** – configurable size (default 512 MB)
- **Move ordering** – TT move, SEE, killer moves, history heuristic
- **Pruning** – null-move pruning, futility pruning, late move reductions
- **Principal Variation Search** with aspiration windows

## Quick Start

