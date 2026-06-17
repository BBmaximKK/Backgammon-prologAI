% ================================================================
% ai.pl — Expectiminimax a profondità 4
%
% Struttura dell'albero:
%
%   [Depth 4]  MAX   ← nostra mossa 1
%                └─ CHANCE  (campioni dadi avversario)
%   [Depth 3]        └─ MIN   ← mossa avversario 1
%                         └─ CHANCE  (campioni nostri dadi)
%   [Depth 2]              └─ MAX   ← nostra mossa 2
%                               └─ CHANCE  (campioni dadi avversario)
%   [Depth 1]                    └─ MIN   ← mossa avversario 2
%                                     └─ FOGLIA
%
% Pipeline applicata a OGNI nodo MAX e MIN
%
%   STEP 1 — genera tutte le mosse legali
%   STEP 2 — assegna TAG a ogni mossa
%   STEP 3 — calcola P_veloce per ogni mossa con i TAG
%            P_veloce = 40*colpo + 35*occupo + 35*salvo + 25*copro
%                     + 20*prima + 15*scappo + 10*race - 30*blot
%   STEP 4 — ordina per P_veloce e tiene solo le top-N
%   STEP 5 — per le top-N: calcola evaluate_move (BV + P completo)
%   STEP 6 — scegli la mossa con score massimo (MAX) o usala
%            come miglior risposta avversaria (MIN)
%
% Fan-out top-N per livello (decresce verso la foglia per contenere
% la complessità computazionale):
%   Depth 4 (MAX1) : top 10
%   Depth 3 (MIN1) : top  7
%   Depth 2 (MAX2) : top  5
%   Depth 1 (MIN2) : top  3
%
% Campioni dadi nei nodi CHANCE: top 5 combinazioni più probabili.
%
% Entry point chiamato da C via SWI-Prolog:
%   choose_best_move(+Board, +Dice, +Player, -BestMove)
% ================================================================

:- module(ai, [
    choose_best_move/4,
    top5_moves/4]).

:- use_module(board).
:- use_module(moves).
:- use_module(evaluation).

% ── Fan-out per livello di profondità ───────────────────────────
top_at_depth(4, 10).
top_at_depth(3, 7).
top_at_depth(2, 5).
top_at_depth(1, 3).

% ── Campioni dadi nei nodi CHANCE ────────────────────────────────
num_dice_samples(5).

% ================================================================
% ENTRY POINT
%   choose_best_move(+Board, +Dice, +Player, -BestMove)
% ================================================================

choose_best_move(Board, Dice, Player, BestMove) :-
    max_node(Board, Dice, Player, 4, BestMove, _Score), !.

% fallback: nessuna mossa disponibile
choose_best_move(_, _, _, no_move).

% ================================================================
% PIPELINE COMUNE: TAG → P_veloce → top-N → evaluate_move
%
%   filter_and_score(+Board, +AllMoves, +Player, +Depth, -ScoredTop)
%
%   Pipeline completa:
%     1. TAG su ogni mossa
%     2. P_veloce con i TAG
%     3. Ordina e taglia top-N
%     4. evaluate_move (BV + P) su ogni mossa rimasta
%   Restituisce lista [Move-FullScore, ...] già ordinata per score
%   decrescente, pronta per MAX (prendi il primo) o MIN (prendi il max).
% ================================================================

filter_and_score(Board, AllMoves, Player, Depth, ScoreTop) :-
    % STEP 2-3: TAG + P_veloce
    tag_and_fast_score(Board, AllMoves, ScoredFast),

    % STEP 4: ordina per P_veloce descrescente, tieni top-N
    sort(2, @>=,ScoredFast, SortedFast),
    top_at_depth(Depth, N),
    take(N, SortedFast, TopN),

    % STEP 5: evaluate_move completa (BV + P) per ogni top candidato
    full_eval_list(Board, TopN, Player, ScoredFull).
 
% ── Calcola evaluate_move per ogni mossa nella lista top-N ───────
full_eval_list(_, [], _, []).
full_eval_list(Board, [Move-_FastScore | Rest], Player,
                [Move-FullScore | Tail]) :-
    evaluation: evaluate_move(Board, Move, Player, FullScore),
    full_eval_list(Board, Rest, Player, Tail).

% ================================================================
% MAX_NODE(+Board, +Dice, +Player, +Depth, -BestMove, -BestScore)
%
%   Nodo MAX: usa la pipeline completa e sceglie la mossa migliore.
%   Poi applica il lookahead avversario (nodo CHANCE → MIN) e
%   calcola lo score finale come:
%     FinalScore = FullScore - 0.7 * AdvExpectedScore
% ================================================================

max_node(Board, Dice, Player, Depth, BestMove, BestScore) :-
    Depth > 0,
    legal_moves(Board, Dice, Player, AllMoves),
    AllMoves \= [], !,

    % Pipeline: TAG → P_veloce → top-N → evaluate_move
    filter_and_score(Board, AllMoves, Player, Depth, ScoredTop),

    % Lookahead avversario per ogni candidato rimasto
    board: opponent(Player, Opp),
    dice_samples(DiceSamples),
    NextDepth is Depth - 1,
    apply_lookahed_max(Board, ScoredTop, Player, Opp,
                        NextDepth, DiceSamples, ScoredFinal),
    
    % Scegle la mossa col punteggio finale massimo
    sort(2, @>=, ScoredFinal, [BestMove-BestScore | _]).

max_node(_, _, _, _, no_move, -9999.0).

% ── Applica lookahead avversario a ogni candidato del MAX ─────────
apply_lookahed_max(_, [], _, _, _, _, []).
apply_lookahed_max(Board, [Move-FullScore | Rest], Player, Opp, NextDepth,
                    DiceSamples, [Move-FinalScore | Tail]) :-
    (NextDepth > 0 ->
        move: apply_move(Board, Move, BoardAfter),
        chance_node_min(BoardAfter, Opp, NextDepth, DiceSamples, AdvExpected),
        FinalScore is FullScore - 0.7 * AdvExpected
    ;
        FinalScore = FullScore),
    apply_lookahed_max(Board, Rest, Player, Opp, NextDepth,
                        DiceSamples, Tail).

% ================================================================
% MIN_NODE(+Board, +Dice, +Opp, +Depth, -BestScore)
%
%   Nodo MIN: usa la pipeline completa dal punto di vista
%   dell'avversario, poi applica il nostro lookahead (CHANCE → MAX).
%   Restituisce il valore massimo per l'avversario (peggio per noi).
% ================================================================
min_node(Board, Dice, Opp, Depth, BestScore) :-
    Depth > 0,
    legal_moves(Board, Dice, Opp, AllMoves),
    AllMoves \= [], !,

    % Pipeline dal punto di vista dell'avversario
    filter_and_score(Board, AllMoves, Opp, Depth, ScoredTop),

    % Lookahead: la nostra risposta dopo la mossa avversaria
    board: opponent(Opp, Player),
    dice_samples(DiceSamples),
    NextDepth is Depth - 1,
    apply_lookahed_max(Board, ScoredTop, Opp, Player, NextDepth,
                        DiceSamples, Scores),
    
    % MIN restituisce il massimo dal punto di vista dell'avversario
    (Scores = [] -> BestScore = 0.0; max_list(Score , BestScore)).

min_node(_, _, _, _, 0.0).

% ── Applica lookahead nostro a ogni candidato del MIN ─────────────
apply_lookahed_max(_, [], _, _, _, _, []).
apply_lookahed_max(Board, [Move-FullScore | Rest], Player, Opp, NextDepth,
                    DiceSamples, [Move-FinalScore | Tail]) :-
    (NextDepth > 0 ->
        moves: apply_move(Board, Move, BoardAfter),
        chance_node_min(BoardAfter, Opp, NextDepth, DiceSamples, AdvExpected),
        FinalScore is FinalScore - 0.7 * AdvExpected
    ;
        FinalScore = FullScore),
    apply_lookahed_max(Board, Rest, Player, Opp, NextDepth,
                        DiceSamples, Tail).

% ================================================================
% MIN_NODE(+Board, +Dice, +Opp, +Depth, -BestScore)
%
%   Nodo MIN: usa la pipeline completa dal punto di vista
%   dell'avversario, poi applica il nostro lookahead (CHANCE → MAX).
%   Restituisce il valore massimo per l'avversario (peggio per noi).
% ================================================================

min_node(Board, Dice, Opp, Depth, BestScore) :-
    Depth > 0,
    legal_moves(Board, Dice, Opp, AllMoves),
    AllMoves \= [], !,

    % Pipeline dal punto di vista dell'avversario
    filter_and_score(Board, AllMoves, Opp, Depth, ScoredTop),

    % Lookahead: nostra risposta dopo la mossa avversaria
    board: opponent(Opp, Player),
    dice_samples(DiceSamples),
    NextDepth is Depth - 1,
    apply_lookahed_min(Board, ScoredTop, Opp, Player, NextDepth,
                        DiceSamples, Scores),
    
    % MIN restituisce il massimo dal punto di vista dell'avversario
    (Scores = [] -> BestScore = 0.0; max_list(Scores, BestScore)).

min_node(_, _, _, _, 0.0).

% ── Applica lookahead nostro a ogni candidato del MIN ─────────────
apply_lookahed_min(_, [], _, _, _, _, []).
apply_lookahed_min(Board, [Move-FullScore | Rest], Opp, Player, NextDepth,
                    DiceSamples, [FinalScore | Tail]) :-
    (NextDepth > 0 ->
        moves:apply_move(Board, Move, BoardAfter),
        chance_node_max(BoardAfter, Player, NextDepth, DiceSamples, OurExpected),
        % l'avversario vuole massimizzare il suo score sottraendo la nostra risposta
        FinalScore is FullScore - 0.6 * OurExpected
    ;
        FinalScore = FullScore),
                    apply_lookahed_min(Board, Rest, Opp, Player, NextDepth,
                                        DiceSamples, Tail).

% ================================================================
% CHANCE_NODE_MIN(+Board, +Opp, +Depth, +Samples, -ExpectedScore)
%
%   Nodo CHANCE dopo una nostra mossa.
%   Il prossimo a muovere è l'avversario → chiama min_node.
%   Calcola la media pesata per probabilità dei dadi campionati.
% ================================================================
chance_node_min(Board, Opp, Depth, DiceSamples, ExpectedScore) :-
    findall(Prob-NodeScore,
            (member(dice(D1, D2) - Prob, DiceSamples),
            min_node(Board, dice(D1, D2), Opp, Depth, NodeScore)),
            ProbScores),
    weighted_avarage(ProbScores, ExpectedScore).

% ================================================================
% CHANCE_NODE_MAX(+Board, +Player, +Depth, +Samples, -ExpectedScore)
%
%   Nodo CHANCE dopo una mossa avversaria.
%   Il prossimo a muovere siamo noi → chiama max_node.
%   Restituisce solo lo score (la mossa scelta non serve qui).
% ================================================================

chance_node_max(Board, Player, Depth, DiceSamples, ExpectedScore) :-
    findall(Prob-NodeScore,
            (member(dice(D1,D2)-Prob, DiceSamples),
             max_node(Board, dice(D1,D2), Player, Depth, _Move, NodeScore)),
            ProbScores),
    weighted_average(ProbScores, ExpectedScore).

% ================================================================
% WEIGHTED_AVERAGE(+[Prob-Score], -Avg)
%   Media pesata: sum(Prob_i * Score_i) / sum(Prob_i)
% ================================================================

weighted_avarage([], 0.0) :- !.
weighted_avarage(ProbScores, Avg) :-
    findall(W, (member(P-S, ProbScores), W is P * S), WS),
    findall(P, member(P - _, ProbScores), Ps),
    sumlist(WS, TotW),
    sumlist(Ps, TotP),
    (TotP =:= 0.0 -> Avg = 0.0 ; Avg is TotW / TotP).

% ================================================================
% TAG + P_VELOCE
%   Produce lista [Move-FastScore, ...] per tutte le mosse.
%   P_veloce = 40*colpo + 35*occupo + 35*salvo + 25*copro
%            + 20*prima + 15*scappo + 10*race - 30*blot
% ================================================================

tag_and_fast_score(_, [], []).
tag_and_fast_score(Board, [M|Ms], [M-S|Rest]) :-
    moves: tag_move(Board, M, Tags),
    evaluation: fast_score(Tags, Board, S),
    tag_and_fast_score(Board, Ms, Rest).

% ================================================================
% CAMPIONI DADI
%   Tutte le coppie (D1, D2) con D1 =< D2, ordinate per probabilità.
%   I non-doppi (2/36) vengono prima dei doppi (1/36).
%   Si prendono le top num_dice_samples.
% ================================================================

dice_samples(Samples) :-
    num_dice_samples(N),
    all_dice_combos(All),
    sort(2, @>=, All, Sorted),
    take(N, Sorted, Samples).

all_dice_combos(Combos) :-
    findall(dice(D1, D2) - Prob,
            (between(D1, 6, D1),
            between(D1, 6, D2),
            (D1 =:= D2 ->
                Prob is 1.0 / 36.0
            ;
                Prob is 2.0 / 36.0)),
            Combos).

% ================================================================
% UTILITY
% ================================================================

take(0, _, []) :- !.
take(_, [], []) :- !.
take(N, [H|T], [H|R]) :-
    N > 0, N1 is N - 1,
    take(N1, T, R).

% ================================================================
% TOP5_MOVES(+Board, +Dice, +Player, -Top5)
%
%   Ritorna le prime 5 mosse con score, nel formato:
%     Top5 = [move(F1,T1,S1), move(F2,T2,S2), ...]
%
%   Usa la stessa pipeline di max_node (TAG → fast → evaluate_move)
%   ma senza lookahead avversario, per velocità.
%   Chiamata da C per popolare il pannello grafico.
% ================================================================

top5_moves(Board, Dice, Player, Top5) :-
    legal_moves(Board, Dice, Player, AllMoves),
    AllMoves \= [], !,
    tag_and_fast_score(Board, AllMoves, ScoredFast),
    sort(2, @>=, ScoredFast, SortedFast),
    take(10, SortedFast, TopFast),
    full_eval_list(Board, TopFast, Player, ScoredFull),
    sort(2, @>=, ScoredFull, SortedFull),
    take(5, SortedFull, Top5Raw),
    % converte in move(From, To, Score) con From/To come interi C-compatibili
    maplist(encode_top_move, Top5Raw, Top5).

top5_moves(_, _, _, []).

% converte move(bar,To) e move(From,off) in indici interi
encode_top_move(move(From, To) - Score, move(FC, TC, Score)) :-
    encode_from(From, FC),
    encode_to(To, TC).

encode_from(bar,  24) :- !.   % IDX_BAR (generico, C distingue per player)
encode_from(F, F).

encode_to(off, 26) :- !.      % IDX_BEAROFF_W (approssimazione, C usa player)
encode_to(T, T).