#pragma once

char *ctss_vm_core_dict =
    ": 2drop drop drop ; "
    ": 2dup over over ; "
    ": 2swap >r -rot r> -rot ; "

    ": 'lit lit lit >dict ; "
    ": '>dict lit >dict >dict ; "
    ": ^immediate immediate! ; immediate! "
    ": compile> 'lit read-token> find >cfa >dict '>dict ; immediate! "

    ": begin here ; immediate! "
    ": until compile> 0branch here - >dict ; immediate! "

    ": ( begin read-token> \" )\" s== until ; immediate! "

    ": if compile> 0branch here 0 >dict ; immediate! "
    ": -save-offset dup here swap - swap ! ; "
    ": then -save-offset ; immediate! "
    ": -save-else-offset swap -save-offset ; "
    ": else compile> branch here 0 >dict -save-else-offset ; immediate! "

    ": 'back-branch compile> branch here - >dict ; "
    ": again 'back-branch ; immediate! "

    ": while compile> 0branch here 0 >dict ; immediate! "
    ": repeat swap 'back-branch -save-offset ; immediate! "

    ": recur compile> branch latest 2 + here - >dict ; immediate! "

    ": val> ( x -- ) read-token> mk-header 'lit >dict compile> ret ; "
    ": set> ( x -- ) read-token> find 3 + ! ; "

    ": var> ( -- addr ) "
    "  here dup dup 1 + here! read-token> mk-header 'lit >dict compile> ret ; "

    ": allot ( n -- addr ) here dup rot + here! ; "
    ": ds>dict! ( ... addr n -- ) "
    "  dup rot + 1 - >r "
    "  begin swap r> dup 1 - >r ! 1 - dup 0 == until "
    "  r> 2drop ; "
    ": dict>ds ( addr n -- ... ) "
    "  >r begin dup @ swap 1 + r> 1 - dup >r 0 == until"
    "  r> 2drop ; "

#ifdef CTSS_VM_FEATURE_PRINT
    ": space 32 emit ; "
    ": cr 10 emit ; "
    ": -see-lit swap 1 + dup @ space .h swap ; "
    ": -see-branch swap 1 + dup @ 2dup space .h + 0x28 emit .h 0x29 emit swap "
    "; "
    ": -see-branch? dup 0x49 == swap 0x4b == or ; "
    ": see> "
    "  read-token> find "
    "  begin "
    "    1 + dup .h 0x3a emit dup @ dup .h "
    "    dup 0x10000 < if dup 1 - word-name else \" docolon\" then print "
    "    dup 4 == if -see-lit then "
    "    dup -see-branch? if -see-branch then "
    "    cr 2 == "
    "  until drop ; "
#endif

    "var> *user-start* "
    "var> *user-prev* "
    ": wipe *user-start* @ here! *user-prev* @ latest! ; "
    "read-token> wipe find swap ! here swap ! ";
