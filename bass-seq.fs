\ setup buffers & operators

" C" " C#" " D" " D#" " E" " F" " F#" " G" " G#" " A" " A#" " B" 12
dup allot dup var> note-names ! swap ds>dict!

: -mk-midi-note ( n base oct ) i>s s+ mk-header 'lit >dict compile> ret ;

: -mk-octave
    >r 0
    begin
        dup ( i i )
        r> dup dup >r ( i i oct oct )
        1 + 12 * ( i i oct note )
        rot ( i oct note i )
        dup ( i oct note i i )
        note-names @ + @ ( i oct note i str )
        -rot ( i oct str note i )
        + ( i oct str note )
        -rot ( i note oct str )
        swap ( i note str oct )
        -mk-midi-note ( i )
        1 + dup ( i i )
        12 ==
    until r> 2drop ;

: -mk-midi-notes 1 begin dup -mk-octave 1 + dup 7 == until drop ;

-mk-midi-notes .vm

C2 E2 G2 A2 G3 E3 C3 C4  8
dup allot dup var> seq ! swap ds>dict!
0.0f var> last-note !

: midi>hz ( note -- freq )
    69 - i>f 12.0f f/ 2.0f swap fpow 440.0f f* ;

: rand-seq-note ( -- freq )
    0 8 rand seq @ + @ 12 + midi>hz ;

\ pitch buffers for osc1/2
buf var> pitch !
buf var> pitch2 !

\ LFO1 = 0.125Hz, oscillating in interval: 0.1 .. 1.9
\ used as time factor for envelope modulation
0.125f hz buf dup var> lfo1pitch ! b! drop
0.9f buf dup var> lfo1amp ! b! drop
1.0f buf dup var> lfo1dc ! b! drop

\ LFO2 = 82.4Hz (MIDI note E0)
\ used for frequence modulation, amplitude reset for each note via `set-pitch` word
82.4f hz buf dup var> lfo2pitch ! b! drop
buf var> lfo2amp !

\ define actual oscillators, env & fx vars
0.0f >osc var> lfo1 !
0.0f >osc var> lfo2 !
0.0f >osc var> osc1 !
0.0f >osc var> osc2 !
0.005f 0.1f 0.25f 0.75f 0.25f >adsr var> env !
0.35f 2.0f >foldback var> fb !

: vfill! ( freq var -- ) @ b! drop ;

\ set freq for both oscillators and lfo2 (which is used for FM)
\ freq for osc2 is slightly detuned
\ freq for lfo2 set to 1 octave lower than input
: set-pitch ( freq -- )
    hz dup dup pitch vfill!
    1.01f f* pitch2 vfill!
    0.5f f* lfo2amp vfill! ;

\ trigger new note: sets pitch and resets envolope
: new-note ( freq -- ) set-pitch env @ reset-adsr ;

\ main update
\ computes 2 LFOs, ADSR, 2 main oscillators, their product sum
\ then passes result through foldback distortion to create more partials
: update ( time -- buf )
    dup last-note @ f- 0.15f >=
    if
        last-note !
        0.0f 1.0f frand 0.6f f<
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
    swap b*! b+! fb @ swap foldback ;

.words