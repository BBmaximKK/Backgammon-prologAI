% ================================================================
% evaluation.pl — Funzione di valutazione completa
%
% Implementa il modello matematico:
%
%  BV  = F_campi + F_prime - F_torri - F_mobilita
%  P   = D + K + F + A - R_blot - T
%  Score = BV + P - delta_pip
% ================================================================

:- module(evaluation, [
        evaluate_board/3,       % evaluate_board(+Board, +Player, -Score)
        evaluate_move/4,        % evaluate_move(+Board, +Move, +Player, -Score)
        fast_score/3,           % fast_score(+Tags, +Board, -Score) euristica veloce
        pip_count/3             % pip_count(+Points, +Player, -Pip)
        ]).

:- use_module(board).

% ================================================================
% PARAMETRI BV
% ================================================================
bv_alpha(0.6).
bv_beta(0.5).
bv_gamma(2.0).
bv_lambda(0.18).
bv_sigma(4.0).
bv_kappa(0.3).
bv_delta(0.9).
bv_eta(0.4).
bv_s(20.0).
bv_rho(0.1).
bv_spread(1.8).      % bonus per ogni campo distinto occupato (N>=2)
bv_tower_cap(4).     % da questa altezza la penalità torre diventa lineare pesante

% PARAMETRI P (move value)
mv_delta(2.5).          % coprire un blot (era 0.5 — troppo basso)
mv_kappa(2.0).          % colpire pedina avversaria
mv_mu(0.65).            % variazione lunghezza prime
mv_alpha(0.55).         % anchor in casa avversaria
mv_beta(2.8).           % penalità R_blot dopo la mossa (era 1.3)
mv_theta(0.3).          % torri eccessive
mv_blot_risk(3.5).      % peso del delta-rischio: quanto migliora la sicurezza

% ================================================================
% PIP COUNT
%   pip_count(+Points, +Player, -Pip)
% ================================================================

pip_count(Points, Player, Pip) :-
    pip_count_loop(Points, Player, 1, 0, Pip).

pip_count_loop([], _, _, Acc, Acc).
pip_count_loop([H|T], white, I, Acc, Pip) :-
    (H > 0 -> V is H * (25 - I); V is 0),
    Acc1 is Acc  + V,
    I1 is I + 1,
    pip_count_loop(T, white, I1, Acc1, Pip).
pip_count_loop([H|T], black, I, Acc, Pip) :-
    (H < 0 -> V is abs(H) * I ; V is 0),
    Acc1 is Acc + V,
    I1 is I + 1,
    pip_count_loop(T, black, I1, Acc1, Pip).

% ================================================================
% FATTORE R(V) — attivazione back-game
%   R(V) = max(0, tanh(rho*(V - s)))
%   V    = pip_avversario - pip_nostro
% ================================================================

r_factor(Points, Player, R) :-
    board: opponent(Player, Opp),
    pip_count(Points, Player, PipNostro),
    pip_count(Points, Opp, PipAvv),
    V is PipAvv - PipNostro,
    bv_rho(Rho), bv_s(S),
    Raw is tanh(Rho * (V - S)),
    R is max(0.0, Raw).

% ================================================================
% O(n) — valore occupazione campo
% ================================================================

o_val(N, 1.0)   :-  N >= 2, !.
o_val(N, 0.2)   :-  N =:= 1, !.
o_val(_, 0.0).

% ================================================================
% T(n) — penalità torre
%   N=1..2 : nessuna penalità (blot e punto difeso sono ok)
%   N=3    : piccola penalità (inizia l'accumulo)
%   N>=4   : cresce rapidamente con (N-2)^2 per scoraggiare torri grandi
% ================================================================

t_val(N, 0.0)  :- N =< 2, !.
t_val(3, 1.0)  :- !.
t_val(N, T)    :- T is float((N - 2) * (N - 2)).

% ================================================================
% B_back(i, b) — bonus blocco back-checker
%   b = indice del pezzo avversario più arretrato
% ================================================================

b_back(I, B, Val) :-
    Diff is B - I,
    (Diff >= 1, Diff =< 6 ->
        Val is 36.0 * float(Diff * Diff)
    ;
        Val is 0.0).

% ================================================================
% W(i) — peso strategico del campo i
%   W(i) = alpha*e^(-lambda*i) + beta*e^(-(i-13)^2/(2*sigma^2))
%          + R(V)*gamma*B_back(i,b)
% ================================================================

w_field(I, R, BackIdx, W) :-
    bv_alpha(A), bv_lambda(Lam),
    bv_beta(Be), bv_sigma(Sig),
    bv_gamma(G),
    FI is float(I),
    W1 is A * exp(-Lam * FI),
    W2 is Be * exp(-(FI - 13.0) * (FI - 13.0) / (2.0 * Sig * Sig)),
    b_back(I, BackIdx, Bb),
    W3 is R * G * Bb,
    W is W1 + W2 + W3.

% ================================================================
% F_campi — somma pesata di tutti i campi del giocatore
% ================================================================

f_campi(Points, Player, R, BackIdx, FCampi) :-
    f_campi_loop(Points, Player, R, BackIdx, 1, 0.0, FCampi).

f_campi_loop([], _, _, _, _, Acc, Acc).
f_campi_loop([H|T], Player, R, BackIdx, I, Acc, FCampi) :- 
    % normalizza: valore sempre positivo per il giocatore corrente
    (Player = white -> N = H ; N is -H),
    (N > 0 -> 
        o_val(N, O),
        w_field(I, R, BackIdx, W),
        Val is W * O
    ;
        Val is 0.0),
    Acc1 is Acc + Val,
    I1 is I + 1,
    f_campi_loop(T, Player, R, BackIdx, I1, Acc1, FCampi).

% ================================================================
% F_torri — penalità torri pesata per W(i)
% ================================================================

f_torri(Points, Player, R, BackIdx, FTorri) :-
    bv_delta(Delta),
    f_torri_loop(Points, Player, R, BackIdx, 1, 0.0, FTorriRaw),
    FTorri is Delta * FTorriRaw.

f_torri_loop([], _, _, _, _, Acc, Acc).
f_torri_loop([H|T], Player, R, BackIdx, I, Acc, FTorri) :-
    (Player = white -> N = H ; N is -H),
    (N > 0 ->
        t_val(N, Tv),
        % usa W con indice specchiato per torre (25-i)
        IMirror is 25 - I,
        w_field(IMirror, R, BackIdx, W),
        Val is W * Tv
    ;
        Val is 0.0),
    Acc1 is Acc + Val,
    I1 is I + 1,
    f_torri_loop(T, Player, R, BackIdx, I1, Acc1, FTorri).

% ================================================================
% F_prime — valore delle prime (sequenze consecutive di punti difesi)
%
% F_prime = kappa * sum_a( L_a^2 * max(0, b - z_a) )
%   L_a   = lunghezza della prima a
%   z_a   = indice finale (più avanzato) della prima a
%   b     = posizione del pezzo avversario più arretrato
% ================================================================

f_prime(Points, Player, BackIdx, FPrime) :-
    bv_kappa(K),
    find_primes(Points, Player, Primes),    % lista di (Lun, Zfin)
    prime_sum(Primes, BackIdx, 0.0, S),
    FPrime is K * S.

% trova tutte le sequenze consecutive di punti difesi (N>=2)
find_primes(Points, Player, Primes) :-
    label_defended([], _, _, []).
    label_defended([H|T], Player, I, [I-D|Rest]) :-
        (Player = white -> N = H ; N is -H),
        (N >= 2 -> D = defended ; D = not),
        I1 is I + 1,
        label_defended(T, Player, I1, Rest).

% estrae run consecutive di difese e calcola (Lunghezza, IndiceFinale)
extract_runs([], []).
extract_runs([I-defended|T], [(Len, IEnd)|Rest]) :-
    collect_run(T, I, I, Len, IEnd, Remaining),
    extract_runs(Remaining, Rest).
extract_runs([_-not|T], Rest) :-
    extract_runs(T, Rest).

collect_run([], _Start, I, Len1, IEnd, Remaining) :- !.
collect_run([I-defended|T], _Start, _PrevEnd, Len, IEnd, Remaining) :-
    !,
    collect_run(T, _Start, I, Len1, IEnd, Remaining),
    Len is Len1 + 1.
collect_run(Rest, _Start, IEnd, 1, IEnd, Rest).

% somma contributi di ogni prima
prime_sum([], _, Acc, Acc).
prime_sum([(Len, Zfin)|T], BackIdx, Acc, S) :-
    Contrib is float(Len * Len) * max(0, BackIdx - Zfin),
    Acc1 is Acc + Contrib,
    prime_sum(T, BackIdx, Acc1, S).

% ================================================================
% F_mobilita — penalità pedine bloccate in torri
%   M = sum_i min(n_i, 3)
%   F_mobilita = eta * (15 - M)
% ================================================================

f_mobilita(Points, Player, FMob) :-
    bv_eta(Eta),
    mob_loop(Points, Player, 0, M),
    FMob is Eta * (15.0 - float(M)).

mob_loop([], _, Acc, Acc).
mob_loop([H|T], Player, Acc, M) :-
    (Player = white -> N = H ; N is -H),
    (N > 0 -> Contrib is min(N, 3) ; Contrib = 0),
    Acc1 is Acc + Contrib,
    mob_loop(T, Player, Acc1, M).

% ================================================================
% F_spread — premia il numero di campi distinti difesi (N>=2)
%   Ogni campo con almeno 2 pedine vale bv_spread punti.
%   Incentiva a distribuire le pedine su più campi invece di
%   accumularle in torri alte.
% ================================================================

f_spread(Points, Player, FSpread) :-
    bv_spread(S),
    count_defended(Points, Player, Count),
    FSpread is S * float(Count).

count_defended([], _, 0).
count_defended([H|T], Player, Count) :-
    (Player = white -> N = H ; N is -H),
    (N >= 2 -> This = 1 ; This = 0),
    count_defended(T, Player, Rest),
    Count is This + Rest.

% ================================================================
% BACK IDX — indice del pezzo avversario più arretrato
%   Per il bianco: cerca il nero con indice più basso
%   Per il nero:   cerca il bianco con indice più alto
% ================================================================

back_idx(Points, white, BackIdx) :-
    findall(I, (nth1(I, Points, V), V < 0), Idxs),
    (Idxs = [] -> BackIdx = 0 ; max_list(Idxs, BackIdx)).

back_idx(Points, black, BackIdx) :-
    findall(I, (nth1(I, Points, V), V > 0), Idxs),
    (Idxs = [] -> BackIdx = 25 ; max_list(Idxs, BackIdx)).

% ================================================================
% evaluate_board(+Board, +Player, -Score)
%   Score = BV = F_campi + F_prime + F_spread - F_torri - F_mobilita
% ================================================================

evaluate_board(board(Points, _, _, OffW, OffB, _), Player, Score) :-
    r_factor(Points, Player, R),
    back_idx(Points, Player, BackIdx),

    f_campi(Points, Player, R, BackIdx, FC),
    f_prime(Points, Player, BackIdx, FP),
    f_spread(Points, Player, FS),
    f_torri(Points, Player, R, BackIdx, FT),
    f_mobilita(Points, Player, FM),

    % bonus pedine uscite
    (Player = white -> Off = OffW ; Off = OffB),
    OffBonus is float(Off) * 3.0,

    Score is FC + FP + FS - FT - FM + OffBonus.

% ================================================================
% MOVE VALUE P
%   P = delta*N_coperti + kappa*N_colpiti + mu*DeltaL_prime
%       + alpha*N_anchor - beta*R_blot - theta*T_eccesso
% ================================================================

% ── B_dadi(d) — probabilità di uscita del valore d ───────────────
b_dadi(D, P) :- D >= 1, D =< 6, !, P is 11.0/36.0.
b_dadi(7,  P) :- !, P is 6.0/36.0.
b_dadi(8,  P) :- !, P is 5.0/36.0.
b_dadi(9,  P) :- !, P is 4.0/36.0.
b_dadi(10, P) :- !, P is 3.0/36.0.
b_dadi(11, P) :- !, P is 2.0/36.0.
b_dadi(12, P) :- !, P is 1.0/36.0.
b_dadi(_, 0.0).

% ── B_colpo(b, Points, Player) — probabilità di essere colpiti ────
% 1 - prod_j(1 - B_dadi(dist(j, blot)))
b_colpo(BlotIdx, Points, Player, Prob) :-
    board: opponent(Player, Opp),
    findall(Dist,
            (nth1(J, Points, V),
            opp_piece(V, Opp),
            Dist is abs(BlotIdx - J),
            Dist >= 1, Dist =< 12),
            Dists),
    prod_miss(Dists, 1.0, ProdMiss),
    Prob is 1.0 - ProdMiss.

opp_piece(V, white) :- V > 0.
opp_piece(V, black) :- V < 0.

prod_miss([], Acc, Acc).
prod_miss([D|T], Acc, Prod) :-
    b_dadi(D, P),
    Acc1 is Acc * (1.0 - P),
    prod_miss(T, Acc1, Prod).

% ── R_blot — rischio totale dei blot dopo la mossa ───────────────
r_blot(Points, Player, RBlot) :-
    findall(Prob,
            (nth1(I, Points, V),
            own_blot(V, Player),
            b_colpo(I, Points, Player, Prob)),
            Probs),
    sumlist(Probs, RBlot).

own_blot(1, white).
own_blot(-1, black).

% ── Delta R_blot — miglioramento di sicurezza prodotto dalla mossa ─
%   Misura quanto il rischio totale dei blot si RIDUCE.
%   Valore positivo = la mossa ha migliorato la sicurezza.
delta_r_blot(OldPoints, NewPoints, Player, Delta) :-
    r_blot(OldPoints, Player, RBefore),
    r_blot(NewPoints, Player, RAfter),
    Delta is RBefore - RAfter.   % > 0 se abbiamo coperto/ridotto blot

% ── N_coperti — blot coperti con questa mossa ────────────────────
n_coperti(OldPoints, NewPoints, Player, N) :-
    findall(1,
            (nth1(I, OldPoints, Vo), nth1(I, NewPoints, Vn),
            own_blot(Vo, Player),       % era blot
            \+ own_blot(Vn, Player)),   % ora non e piu blot
            L),
    length(L, N).

% ── N_colpiti — pedine avversarie mandate al bar ─────────────────
n_colpiti(OldPoints, NewPoints, Player, N) :-
    board:opponent(Player, Opp),
    findall(1,
            (nth1(I, OldPoints, Vo), nth1(I, NewPoints, Vn),
             opp_piece(Vo, Opp),       % era pezzo avversario
             \+ opp_piece(Vn, Opp)),   % ora non c'è più
            L),
    length(L, N).
 
% ── DeltaL_prime — variazione lunghezza prime ────────────────────
delta_l_prime(OldPoints, NewPoints, Player, DL) :-
    total_prime_len(OldPoints, Player, L0),
    total_prime_len(NewPoints, Player, L1),
    DL is L1 - L0.
 
total_prime_len(Points, Player, Total) :-
    find_primes(Points, Player, Primes),
    findall(L, member((L,_), Primes), Lens),
    sumlist(Lens, Total).

% ── N_anchor — anchor creati (punti difesi in casa avversaria) ────
n_anchor(OldPoints, NewPoints, white, N) :-
    findall(1,
            (nth1(I, NewPoints, Vn), Vn >= 2, I >= 19,  % casa nera per bianco
             nth1(I, OldPoints, Vo), Vo < 2),
            L),
    length(L, N).
n_anchor(OldPoints, NewPoints, black, N) :-
    findall(1,
            (nth1(I, NewPoints, Vn), Vn =< -2, I =< 6,  % casa bianca per nero
             nth1(I, OldPoints, Vo), Vo > -2),
            L),
    length(L, N).

% ── T_eccesso — torri create dalla mossa ─────────────────────────
t_eccesso(NewPoints, Player, T) :-
    findall(E,
            (nth1(_, NewPoints, V),
             (Player = white -> N = V ; N is -V),
             N > 3,
             E is N - 3),
            Ls),
    sumlist(Ls, T).

% ================================================================
% evaluate_move(+OldBoard, +Move, +Player, -Score)
%   Score = BV_new + P
% ================================================================
 
evaluate_move(OldBoard, Move, Player, Score) :-
    OldBoard = board(OldPoints, _, _, _, _, _),
    moves: apply_move(OldBoard, Move, NewBoard),
    NewBoard = board(NewPoints, _, _, _, _, _),

    % BV della nuova posizione
    evaluate_board(NewBoard, Player, BV),

    % P = valore della mossa singola
    mv_delta(MVD), mv_kappa(MVK), mv_mu(MVMu),
    mv_alpha(MVA), mv_beta(MVB), mv_theta(MVT),
    mv_blot_risk(MVBR),

    n_coperti(OldPoints, NewPoints, Player, NCop),
    n_colpiti(OldPoints, NewPoints, Player, NCol),
    delta_l_prime(OldPoints, NewPoints, Player, DL),
    n_anchor(OldPoints, NewPoints, Player, NAn),
    r_blot(NewPoints, Player, RB),
    delta_r_blot(OldPoints, NewPoints, Player, DRB),
    t_eccesso(NewPoints, Player, TE),

    P is MVD  * float(NCop)       % bonus diretto per blot coperti
       + MVK  * float(NCol)       % bonus colpo avversario
       + MVMu * float(DL)         % variazione prime
       + MVA  * float(NAn)        % anchor in casa avversaria
       + MVBR * DRB               % bonus per riduzione rischio blot (delta)
       - MVB  * RB                % penalità rischio blot residuo
       - MVT  * float(TE),        % penalità torri eccessive

    % delta PIP
    pip_count(OldPoints, Player, PipOld),
    pip_count(NewPoints, Player, PipNew),
    DeltaPip is float(PipOld - PipNew),

    Score is BV + P + 0.1 * DeltaPip.

% ================================================================
% EURISTICA VELOCE — punteggio tag-based
%
%   Tag            Peso   Ragione
%   ─────────────  ─────  ──────────────────────────────────────
%   colpo          +45    colpire è la mossa più forte
%   nuovo_punto    +40    creare un punto difeso nuovo è ottimo
%   copro          +38    coprire un blot è quasi uguale
%   occupo         +30    occupare campo vuoto
%   salvo          +25    mettere in salvo un blot
%   race           +12    bear-off / rientro (situazionale)
%   blot           -25    lasciare un blot è rischioso
%   impila         -35    accumulare su campo già difeso è male
%   torre          -55    costruire torri alte è molto male
% ================================================================

fast_score(Tags, _Board, Score) :-
    findall(W, (member(Tag, Tags), tag_weight(Tag, W)), Ws),
    sumlist(Ws, Score).

tag_weight(colpo,       45).
tag_weight(nuovo_punto, 40).
tag_weight(copro,       38).
tag_weight(occupo,      30).
tag_weight(salvo,       25).
tag_weight(race,        12).
tag_weight(blot,       -25).
tag_weight(impila,     -35).
tag_weight(torre,      -55).