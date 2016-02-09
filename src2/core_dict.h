#pragma once

#define NL "\n"

char *ctss_vm_core_dict =
  ": 'lit lit lit push-dict ;" NL
  ": 'push-dict lit push-dict push-dict ;" NL
  ": ^immediate immediate! ; immediate!" NL
  ": compile> 'lit read-token> find >cfa push-dict 'push-dict ; immediate!" NL

  ": if compile> 0branch dict-here 0 push-dict ; immediate!" NL
  ": save-offset dup dict-here swap - swap ! ;" NL
  ": then save-offset ; immediate!" NL
  ": save-else-offset swap save-offset ;" NL
  ": else compile> branch dict-here 0 push-dict save-else-offset ; immediate!" NL

  ": begin dict-here ; immediate!" NL
  ": until compile> 0branch dict-here - push-dict ; immediate!" NL

  ": 'back-branch compile> branch dict-here - push-dict ;" NL
  ": again 'back-branch ; immediate!" NL

  ": while compile> 0branch dict-here 0 push-dict ; immediate!" NL
  ": repeat swap 'back-branch save-offset ; immediate!" NL

  ": recur compile> branch latest 2 + dict-here - push-dict ; immediate!" NL

  ": val> read-token> create-header 'lit push-dict compile> ret ;";

#undef NL
