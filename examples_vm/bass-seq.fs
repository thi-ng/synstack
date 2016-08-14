\ define scale for sequencer
C1 C1 C1 C1 C2 C2 C2 C2 C4 9
dup allot dup var> seq ! swap ds>dict!

: rand-seq-note ( -- freq ) 0 9 rand seq @ + @ 2.0f f* ;

\ pitch buffers for osc1/2
buf var> pitch !
buf var> pitch2 !

\ LFO1 = 0.125Hz, oscillating in interval: 0.1 .. 1.9
\ used as time factor for envelope modulation
0.125f hz buf dup var> lfo1pitch ! b! drop
0.9f buf dup var> lfo1amp ! b! drop
1.0f buf dup var> lfo1dc ! b! drop

\ LFO2 = 82.4Hz (MIDI note E2)
\ used for frequence modulation, amplitude reset for each note via `set-pitch` word
C2 hz buf dup var> lfo2pitch ! b! drop
buf var> lfo2amp !

\ define actual oscillators, env & fx vars
0.0f >osc var> lfo1 !
0.0f >osc var> lfo2 !
0.0f >osc var> osc1 !
0.0f >osc var> osc2 !
0.005f 0.1f 0.75f 0.8f 0.25f >adsr var> env !
0.33f 2.5f >foldback var> fb !
0.0f var> last-note !

: vfill! ( freq var -- ) @ b! drop ;

\ set freq for both oscillators and lfo2 (which is used for FM)
\ freq for osc2 is slightly detuned
\ freq for lfo2 set to 1 octave lower than input
: set-pitch ( freq -- )
    hz dup dup pitch vfill!
    1.01f f* pitch2 vfill!
    0.9f f* lfo2amp vfill! ;

\ trigger new note: sets pitch and resets envolope
: new-note ( freq -- ) set-pitch env @ reset-adsr ;

\ main update
\ computes 2 LFOs, ADSR, 2 main oscillators, their product sum
\ then passes result through foldback distortion to create more partials
: update ( time -- buf )
    dup last-note @ f- 0.15f f>=
    if
        last-note !
        0.0f 1.0f frand 0.8f f<
        if rand-seq-note new-note then
    else
        drop
    then
    lfo1 @ lfo1pitch @ osc-sin lfo1amp @ b*! lfo1dc @ b+!
    env @ swap adsr dup
    lfo2 @ lfo2pitch @ osc-square lfo2amp @ b*! pitch @ b+!
    osc1 @ swap osc-sin
    swap b*! swap
    osc2 @ pitch2 @ osc-sin
    swap b*! b+!
    fb @ swap foldback
;

\ .vm .words .mem
