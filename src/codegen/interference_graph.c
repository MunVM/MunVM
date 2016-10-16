#include <mun/codegen/interference_graph.h>
#include <mun/location.h>
#include <mun/array.h>
#include <mun/type.h>
#include "all.h"

void
interference_graph_init(interference_graph* rig, flow_graph* cfg){
  rig->postorder = &cfg->postorder;
  rig->graph = cfg;
  array_init(&rig->instructions, 0xA);
  array_init(&rig->live_ranges, cfg->current_ssa_temp_index);
  array_init(&rig->active, cfg->current_ssa_temp_index);
  array_init(&rig->register_pool, kNumberOfCpuRegisters);
  array_init(&rig->register_maps, kNumberOfCpuRegisters);
  for(word i = 0; i <= kNumberOfCpuRegisters; i++) {
    array_add(&rig->register_maps, wdup(0));
    array_add(&rig->register_pool, wdup(i + 1));
  }
}

static void
number(interference_graph* rig){
  word pos = 0x0;

  for(word i = rig->postorder->size - 1; i >= 0; i--){
    block_entry_instr* block = rig->postorder->data[i];

    array_add(&rig->instructions, block);

    block->start_pos = pos;
    ((instruction*) block)->lifetime_pos = pos;
    pos += 2;

    FORWARD_ITER(block){
      instr_initialize_location_summary(it);
      if(it->type != kParallelMoveInstr){
        array_add(&rig->live_ranges, NULL);
        array_add(&rig->instructions, it);
        printf("%s ==> %li\n", instr_name(it), pos);
        it->lifetime_pos = pos;
        pos += 2;
      }
    }

    block->end_pos = pos;
  }
}

static void
process(interference_graph* rig, instruction* instr){
  for(word i = 0; i < instr_input_count(instr); i++){
    input* in = instr_input_at(instr, i);

    live_range* range = rig->live_ranges.data[in->defn->ssa_temp_index];
    if(range == NULL){
      range = rig->live_ranges.data[in->defn->ssa_temp_index] = malloc(sizeof(live_range));
      range->start = range->end = instr->lifetime_pos;
      range->vreg = in->defn->ssa_temp_index;
    } else{
      range->end = instr->lifetime_pos;
    }
  }

  if(instr_is_definition(instr)){
    definition* defn = container_of(instr, definition, instr);
    live_range* range = rig->live_ranges.data[defn->ssa_temp_index];
    if(range == NULL){
      range = rig->live_ranges.data[defn->ssa_temp_index] = malloc(sizeof(live_range));
      range->start = range->end = instr->lifetime_pos;
      range->vreg = defn->ssa_temp_index;
    } else{
      range->end = instr->lifetime_pos;
    }
  }
}

static void
build_live_ranges(interference_graph* rig){
  for(word i = 0; i < rig->postorder->size; i++){
    block_entry_instr* block = rig->postorder->data[i];

    instruction* current = block->last;
    while(current != ((instruction*) block)){
      process(rig, current);
      current = current->prev;
    }
  }
}

MUN_INLINE void
add_to_active(interference_graph* rig, live_range* range){
  word pos = 0x0;
  for(word index = 0x0; index < rig->active.size; index++){
    live_range* active = rig->active.data[index];
    if(active->end < range->end){
      pos = index;
      break;
    }
  }
  array_insert(&rig->active, pos, range);
}

MUN_INLINE word
alloc_register(interference_graph* rig, live_range* range){
  word* regi = array_pop(&rig->register_pool);
  add_to_active(rig, range);
  return *regi;
}

static void
alloc_ranges(interference_graph* rig){
  word spill_count = 0x0;
  for(word index = 0x0; index < rig->live_ranges.size; index++){
    live_range* range = rig->live_ranges.data[index];
    if(range != NULL){
      if(rig->active.size > 0){
        for(word i = 0; i < rig->active.size; i++){
          live_range* active = rig->active.data[i];
          if(active->end <= range->start) break;
          array_add(&rig->register_pool, rig->register_maps.data[active->vreg]);
          rig->active.data[i] = NULL;
        }
      }

      if(rig->register_pool.size == 0x0){
        live_range* spill = array_last(&rig->active);
        if(spill->end > range->end){
          array_pop(&rig->active);

          add_to_active(rig, range);
          rig->register_maps.data[range->vreg] = rig->register_maps.data[spill->vreg];
          *((word*) rig->register_maps.data[spill->vreg]) = -(spill_count++);
        } else{
          *((word*) rig->register_maps.data[range->vreg]) = -(spill_count++);
        }
      } else{
        word regi = alloc_register(rig, range);
        *((word*) rig->register_maps.data[range->vreg]) = -(regi - kNumberOfCpuRegisters);
      }
    }
  }
}

static void
assign(interference_graph* rig) {
  for(word i = rig->postorder->size - 1; i >= 0; i--){
    block_entry_instr* block = rig->postorder->data[i];
    BACKWARD_ITER(block){
      if(it->locations == NULL) continue;
      location_summary* locs = it->locations;
      for(word j = 0; j < instr_input_count(it); j++){
        input* in = instr_input_at(it, j);
        word assignment = *((word*) rig->register_maps.data[in->defn->ssa_temp_index]);
        loc_init_r(&locs->inputs[j], ((asm_register) assignment));
      }

      if(loc_get_policy(locs->output) == kSameAsFirstInput){
        locs->output = locs->inputs[0];
      } else{
        definition* defn = container_of(it, definition, instr);
        word assignment = *((word*) rig->register_maps.data[defn->ssa_temp_index]);
        printf("$%li: %s -> %s\n", defn->ssa_temp_index, instr_name(it), asm_registers[assignment]);
        loc_init_r(&locs->output, ((asm_register) assignment));
      }
    }
  }
}

void
interference_graph_compute(interference_graph* rig){
  number(rig);
  build_live_ranges(rig);
  alloc_ranges(rig);
  assign(rig);
}