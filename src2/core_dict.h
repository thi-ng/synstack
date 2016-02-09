#pragma once

#define NL "\n"

char *ctss_vm_core_dict =
  ": 'lit lit lit >dict ;" NL
  ": '>dict lit >dict >dict ;" NL
  ": ^immediate immediate! ; immediate!" NL
  ": compile> 'lit read-token> find >cfa >dict '>dict ; immediate!" NL

  ": if compile> 0branch here 0 >dict ; immediate!" NL
  ": -save-offset dup here swap - swap ! ;" NL
  ": then -save-offset ; immediate!" NL
  ": -save-else-offset swap -save-offset ;" NL
  ": else compile> branch here 0 >dict -save-else-offset ; immediate!" NL

  ": begin here ; immediate!" NL
  ": until compile> 0branch here - >dict ; immediate!" NL

  ": 'back-branch compile> branch here - >dict ;" NL
  ": again 'back-branch ; immediate!" NL

  ": while compile> 0branch here 0 >dict ; immediate!" NL
  ": repeat swap 'back-branch -save-offset ; immediate!" NL

  ": recur compile> branch latest 2 + here - >dict ; immediate!" NL

  ": val> read-token> mk-header 'lit >dict compile> ret ;" NL

  "here 16 + val> *user-start*" NL
  ": wipe *user-start* dup here! 11 - latest! ;";

#undef NL
