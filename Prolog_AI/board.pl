% ================================================================
% board.pl — stato della tavola
% Board = board(Points, BarW, BarB, OffW, OffB, Turn)
%   Points : lista 24 interi
%            > 0  pedine BIANCHE
%            < 0  pedine NERE
%            = 0  campo vuoto
%   BarW/BarB : pedine sul bar
%   OffW/OffB : pedine uscite (bear-off)
%   Turn      : white | black
% ================================================================

:- module(board, [
        initial_board/1,
        switch_player/2,
        game_over/2,
        opponent/2]).

% ── Tavola iniziale ────────────────────────────────────────────────
initial_board(board(
    [ 2, 0, 0, 0, 0,-5,
      0,-3, 0, 0, 0, 5,
     -5, 0, 0, 0, 3, 0,
      5, 0, 0, 0, 0,-2],
    0, 0, 0, 0, white)).

% ── Cambio turno ──────────────────────────────────────────────────
switch_player(white, black).
switch_player(black, white).

opponent(white, black).
opponent(black, white).

% ── Fine partita ──────────────────────────────────────────────────
game_over(board(_,_,_,15,_,_), white).
game_over(board(_,_,_,_,15,_), black).