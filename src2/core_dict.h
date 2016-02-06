#pragma once

#define NL "\n"

char *ctss_vm_core_dict = "" \
  ": 'lit lit lit push-dict ;" NL \
  ": 'push-dict lit push-dict push-dict ;" NL \
  ": compile> 'lit read-token> find >cfa push-dict 'push-dict ; immediate!" NL \
  "" NL \
  ": if compile> 0branch dict-here 0 push-dict ; immediate!" NL \
  ": save-offset dup dict-here swap - swap ! ;" NL \
  ": then save-offset ; immediate!" NL \
  ": save-else-offset swap save-offset ;" NL \
  ": else compile> branch dict-here 0 push-dict save-else-offset ; immediate!";
