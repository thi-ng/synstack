#pragma once

char *ctss_synth_dict =
    "\" C\" \" C#\" \" D\" \" D#\" \" E\" \" F\" \" F#\" \" G\" \" G#\" \" A\" "
    "\" A#\" \" B\" 12 "
    "dup allot dup var> -note-names ! swap ds>dict! "

    // http://newt.phys.unsw.edu.au/jw/notes.html
    ": midi>hz ( note -- freq ) 69 - i>f 12.0f f/ 2.0f swap fpow 440.0f f* ; "

    ": -oct-note ( oct i -- note ) swap 1 + 12 * + ; "
    ": -oct-note-freq ( oct i -- note ) -oct-note midi>hz ; "
    ": -oct-note-name ( oct i -- str ) -note-names @ + @ swap i>s s+ ; "

    ": -mk-note ( oct i ) 2dup -oct-note-freq -rot -oct-note-name mk-header "
    "'lit >dict compile> ret ; "

    ": -mk-octave ( oct -- ) 0 begin 2dup -mk-note 1 + dup 12 == until 2drop ; "

    ": -mk-midi-notes 0 begin dup -mk-octave 1 + dup 8 == until drop ; "

    "-mk-midi-notes";
