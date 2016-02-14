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

    "here 17 + var> *user-start* ! "
    ": wipe *user-start* @ dup here! 11 - latest! ; ";
