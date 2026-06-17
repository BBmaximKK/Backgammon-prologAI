% ================================================================
% moves.pl — generazione mosse legali
%
% Convenzione indici (lista 1-based, come Prolog nth1):
%   Bianco: si muove da indici ALTI verso BASSI (24 → 1 → bear-off)
%   Nero  : si muove da indici BASSI verso ALTI  (1 → 24 → bear-off)
%
% move(From, To)   : mossa normale
% move(bar, To)    : rientro dal bar
% move(From, off)  : bear-off
% ================================================================

:- module(moves, [
    legal_moves/4,
    apply_move/3,
    tag_move/3
]).

:- use_module(board).

% ── Costanti direzione ─────────────────────────────────────────────
direction(white, -1).   % bianco scende
direction(black,  1).   % nero sale

% ================================================================
% legal_moves(+Board, +Dice, +Player, -Moves)
%   Dice = dice(D1, D2) oppure dice(D1,D1,D1,D1) per i doppi
%   Genera TUTTE le mosse legali per la coppia di dadi
% ================================================================
legal_moves(Board, dice(D1, D2), Player, Moves) :-
    D1 =\= D2, !,
    all_single_moves(Board, [D1, D2], Player, Moves).

legal_moves(Board, dice(D1, D1), Player, Moves) :-
    all_single_moves(Board, [D1, D1, D1, D1], Player, Moves).

% Raccoglie mosse per ogni dado nella lista
all_single_moves(Board, Dice, Player, AllMoves) :-
    findall(move(F, T),
            (member(D, Dice),
             single_move(Board, Player, D, F, T)),
            AllMoves).

% ================================================================
% single_move(+Board, +Player, +Die, -From, -To)
%   Una singola mossa legale con dado D
% ================================================================

% ── Rientro dal bar ha priorità assoluta ─────────────────────────
single_move(board(Points, BarW, _, _, _, _), white, D, bar, To) :-
    BarW > 0,
    To is 25 - D,           % bianco rientra in casa nera (punti 19-24)
    To >= 1, To =< 24,
    nth1(To, Points, Dest),
    Dest >= -1.             % non bloccato da >= 2 nere

single_move(board(Points, _, BarB, _, _, _), black, D, bar, To) :-
    BarB > 0,
    To is D,                % nero rientra in casa bianca (punti 1-6)
    To >= 1, To =< 24,
    nth1(To, Points, Dest),
    Dest =< 1.              % non bloccato da >= 2 bianche

% ── Mossa normale (nessuno sul bar) ──────────────────────────────
single_move(board(Points, 0, _, _, _, _), white, D, From, To) :-
    nth1(From, Points, N),
    N > 0,
    To is From - D,
    To >= 1, To =< 24,
    nth1(To, Points, Dest),
    Dest >= -1.

single_move(board(Points, _, 0, _, _, _), black, D, From, To) :-
    nth1(From, Points, N),
    N < 0,
    To is From + D,
    To >= 1, To =< 24,
    nth1(To, Points, Dest),
    Dest =< 1.

% ── Bear-off ─────────────────────────────────────────────────────
single_move(board(Points, 0, _, _, _, _), white, D, From, off) :-
    all_in_home(Points, white),
    nth1(From, Points, N),
    N > 0,
    From =< 6,              % casa del bianco: punti 1-6
    To is From - D,
    (To =:= 0 ;             % dado esatto
     (To < 0, no_higher_white(Points, From))).  % dado eccedente

single_move(board(Points, _, 0, _, _, _), black, D, From, off) :-
    all_in_home(Points, black),
    nth1(From, Points, N),
    N < 0,
    From >= 19,             % casa del nero: punti 19-24
    To is From + D,
    (To =:= 25 ;
     (To > 25, no_higher_black(Points, From))).

% ── Tutti i pezzi in casa? ────────────────────────────────────────
all_in_home(Points, white) :-
    forall((nth1(I, Points, V), V > 0), I =< 6).

all_in_home(Points, black) :-
    forall((nth1(I, Points, V), V < 0), I >= 19).

% ── Nessun pezzo su campo più alto (bear-off con dado eccedente) ──
no_higher_white(Points, From) :-
    \+ (nth1(I, Points, V), V > 0, I > From).

no_higher_black(Points, From) :-
    \+ (nth1(I, Points, V), V < 0, I < From).

% ================================================================
% apply_move(+Board, +Move, -NewBoard)
%   Applica la mossa e restituisce la nuova board
% ================================================================

% Rientro dal bar - bianco
apply_move(board(Points, BarW, BarB, OffW, OffB, T),
           move(bar, To),
           board(NewPoints, NBarW, BarB, OffW, OffB, T)) :-
    NBarW is BarW - 1,
    nth1(To, Points, Dest),
    (Dest =:= -1 ->                         % colpo!
        set_point(Points, To, 1, P1),
        BarB1 is BarB + 1,
        set_bar(BarB1, P1, NewPoints)
    ;
        modify_point(Points, To, 1, NewPoints)
    ).

% Rientro dal bar - nero
apply_move(board(Points, BarW, BarB, OffW, OffB, T),
           move(bar, To),
           board(NewPoints, BarW, NBarB, OffW, OffB, T)) :-
    NBarB is BarB - 1,
    nth1(To, Points, Dest),
    (Dest =:= 1 ->
        set_point(Points, To, -1, P1),
        BarW1 is BarW + 1,
        set_bar(BarW1, P1, NewPoints)
    ;
        modify_point(Points, To, -1, NewPoints)
    ).

% Bear-off bianco
apply_move(board(Points, BarW, BarB, OffW, OffB, T),
           move(From, off),
           board(NewPoints, BarW, BarB, NOffW, OffB, T)) :-
    nth1(From, Points, N),
    N > 0,
    NOffW is OffW + 1,
    modify_point(Points, From, -1, NewPoints).  % togli 1 bianca

% Bear-off nero
apply_move(board(Points, BarW, BarB, OffW, OffB, T),
           move(From, off),
           board(NewPoints, BarW, BarB, OffW, NOffB, T)) :-
    nth1(From, Points, N),
    N < 0,
    NOffB is OffB + 1,
    modify_point(Points, From, 1, NewPoints).   % togli 1 nera (val diventa meno negativo)

% Mossa normale bianco
apply_move(board(Points, BarW, BarB, OffW, OffB, T),
           move(From, To),
           board(NewPoints, BarW, NBarB, OffW, OffB, T)) :-
    integer(From), integer(To),
    nth1(From, Points, Src), Src > 0,
    nth1(To, Points, Dst),
    % togli pedina dalla sorgente
    modify_point(Points, From, -1, P1),
    (Dst =:= -1 ->                          % colpo!
        set_point(P1, To, 1, P2),
        NBarB is BarB + 1
    ;
        modify_point(P1, To, 1, P2),
        NBarB = BarB
    ),
    NewPoints = P2.

% Mossa normale nero
apply_move(board(Points, BarW, BarB, OffW, OffB, T),
           move(From, To),
           board(NewPoints, NBarW, BarB, OffW, OffB, T)) :-
    integer(From), integer(To),
    nth1(From, Points, Src), Src < 0,
    nth1(To, Points, Dst),
    modify_point(Points, From, 1, P1),
    (Dst =:= 1 ->
        set_point(P1, To, -1, P2),
        NBarW is BarW + 1
    ;
        modify_point(P1, To, -1, P2),
        NBarW = BarW
    ),
    NewPoints = P2.

% ── Utility lista ────────────────────────────────────────────────
% modify_point: aggiunge Delta al punto Index (1-based)
modify_point(Points, Index, Delta, NewPoints) :-
    nth1(Index, Points, Old),
    New is Old + Delta,
    replace_nth1(Points, Index, New, NewPoints).

set_point(Points, Index, Val, NewPoints) :-
    replace_nth1(Points, Index, Val, NewPoints).

replace_nth1([_|T], 1, X, [X|T]) :- !.
replace_nth1([H|T], I, X, [H|R]) :-
    I > 1, I1 is I - 1,
    replace_nth1(T, I1, X, R).

set_bar(_, Points, Points).   % placeholder - barra gestita nel campo

% ================================================================
% tag_move(+Board, +Move, -Tags)
%   Assegna tag alla mossa per l'euristica veloce.
%
%   Tag          Significato
%   ──────────── ───────────────────────────────────────────────
%   colpo        colpiamo una pedina avversaria solitaria
%   copro        copriamo un nostro blot (campo: 1 → 2)
%   occupo       occupiamo un campo vuoto (campo: 0 → 1... ma
%                  diventa difeso solo se un'altra pedina arriva)
%   nuovo_punto  portiamo un campo da 1 a 2 (creiamo punto difeso)
%   impila       portiamo un campo da N>=3 a N+1 (accumulo inutile)
%   torre        portiamo un campo da N>=5 a N+1 (torre pericolosa)
%   salvo        salviamo un nostro blot spostandolo
%   blot         lasciamo un blot partendo da un campo con N=2..3
%   race         bear-off o rientro dal bar
% ================================================================
tag_move(board(Points, _, _, _, _, _), move(From, To), Tags) :-
    findall(Tag, get_tag(Points, From, To, Tag), Tags).

% ── colpo: destinazione è una pedina avversaria solitaria ────────
get_tag(Points, _From, To, colpo) :-
    integer(To),
    nth1(To, Points, V), V =:= -1.

% ── copro: copro un mio blot (campo da 1 a 2) ────────────────────
get_tag(Points, _From, To, copro) :-
    integer(To),
    nth1(To, Points, V), V =:= 1.

% ── nuovo_punto: porto un campo da 1 a 2 (punto difeso nuovo) ────
%    (si sovrappone a copro per il bianco, qui generico)
get_tag(Points, _From, To, nuovo_punto) :-
    integer(To),
    nth1(To, Points, V), V =:= 1.

% ── occupo: occupo un campo completamente vuoto ──────────────────
get_tag(Points, _From, To, occupo) :-
    integer(To),
    nth1(To, Points, V), V =:= 0.

% ── impila: aggiungo su un campo già con 3 o 4 pedine ────────────
get_tag(Points, _From, To, impila) :-
    integer(To),
    nth1(To, Points, V), V >= 3, V =< 4.

% ── torre: aggiungo su un campo già con 5+ pedine ────────────────
get_tag(Points, _From, To, torre) :-
    integer(To),
    nth1(To, Points, V), V >= 5.

% ── salvo: sposto un mio blot solitario ──────────────────────────
get_tag(Points, From, _To, salvo) :-
    integer(From),
    nth1(From, Points, V), V =:= 1.

% ── blot: parto da un campo con 2 o 3 pedine, lascio un blot ─────
get_tag(Points, From, _To, blot) :-
    integer(From),
    nth1(From, Points, V), V >= 2, V =< 3.

% ── race: bear-off o rientro dal bar (solo questi due casi) ───────
get_tag(_Points, bar, _To, race) :- !.
get_tag(_Points, _From, off, race) :- !.