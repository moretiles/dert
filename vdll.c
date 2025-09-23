#include "vdll.h"
#include "vdll_priv.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

Vdll_node *vdll_node_init(size_t elem_size){
	if(elem_size == 0){
		return NULL;
	}

	Vdll_node *ret = malloc(elem_size + sizeof(Vdll_node));
	if(ret == NULL){
		return NULL;
	}

	ret->data = (((void*) ret) + sizeof(Vdll_node));
	ret->prev = NULL;
	ret->next = NULL;
	return ret;
}

int vdll_node_destroy(Vdll_node *node){
	if(node != NULL){
		// memfill here just in case there is dangling pointer
		memset(node, 0, sizeof(Vdll_node));
		free(node);
	}

	return 0;
}

int vdll_seek(Vdll *dll, size_t pos){
	if(dll == NULL){
		return 1;
	}

	if(!(dll->pos == 0 && dll->cap == 0 && pos == 0) && pos >= dll->cap){
		return 2;
	}

	void *orig_ptr = dll->ptr;
	size_t orig_pos = dll->pos;

	int check = 0;
	if(pos > dll->pos){
		check = vdll_wind(dll, pos - dll->pos);
	} else if(pos < dll->pos){
		check = vdll_rewind(dll, dll->pos - pos);
	}

	if(check != 0){
		dll->ptr = orig_ptr;
		dll->pos = orig_pos;
		return 3;
	}

	return 0;
}

int vdll_wind(Vdll *dll, size_t increment){
	if(dll == NULL || dll->ptr == NULL){
		return 1;
	}

	size_t i;
	for(i = 0; i < increment && (i + 1) < dll->cap; i++){
		dll->ptr = dll->ptr->next;
		dll->pos += 1;
	}

	if (i != increment){
		return 2;
	}

	return 0;
}

int vdll_rewind(Vdll *dll, size_t decrement){
	if(dll == NULL || dll->ptr == NULL){
		return 1;
	}

	size_t i;
	for(i = decrement; i >= 1 && dll->pos > 0; i--){
		dll->ptr = dll->ptr->prev;
		dll->pos -= 1;
	}

	if (i != 0){
		return 2;
	}

	return 0;
}

Vdll *vdll_init(size_t elem_size, Vdll_functions *functions){
	Vdll *ret = malloc(sizeof(Vdll));
	if(ret == 0){
		return NULL;
	}

	ret->ptr = NULL;
	ret->elem_size = elem_size;
	ret->pos = 0;
	ret->cap = 0;
	ret->functions = functions;
	return ret;
}

int vdll_get(Vdll *dll, size_t pos, void *dest){
	if(dll == NULL || dest == NULL){
		return 1;
	}

	if(vdll_seek(dll, pos) != 0){
		return 2;
	}

	if(memcpy(dest, dll->ptr->data, dll->elem_size) != dest){
		return 3;
	}

	return 0;
}

int vdll_set(Vdll *dll, size_t pos, void *src){
	if(dll == NULL || src == NULL){
		return 1;
	}

	if(vdll_seek(dll, pos) != 0){
		return 2;
	}

	if(memcpy(dll->ptr->data, src, dll->elem_size) != dll->ptr->data){
		return 2;
	}

	return 0;
}

int vdll_destroy(Vdll *dll){
	if(dll == NULL){
		return 1;
	}

	if(dll->cap != 0){
		if(vdll_shrink(dll, dll->cap) != 0){
			return 2;
		}
	}

	free(dll);
	return 0;
}

size_t vdll_len(Vdll *dll){
	if(dll == NULL){
		return 0;
	} else {
		return dll->cap;
	}
}

int vdll_insert(Vdll *dll, size_t pos, size_t num_elems){
	if(dll == NULL || num_elems == 0){
		return 1;
	}

	if(vdll_seek(dll, pos) != 0){
		return 2;
	}

	for(size_t i = 0; i < num_elems; i++){
		Vdll_node *new = vdll_node_init(dll->elem_size);
		if(new == NULL){
			if(i > 1){
				// deallocate everything added prior to failure
				vdll_delete(dll, pos + 1, i);
			}
			return 3;
		}

		if(dll->ptr == NULL){
			dll->ptr = new;
		} else {
			new->prev = dll->ptr;
			new->next = dll->ptr->next;
			if(dll->ptr->next){
				dll->ptr->next->prev = new;
			}
			dll->ptr->next = new;
		}

		dll->cap += 1;
	}

	return 0;
}

int vdll_delete(Vdll *dll, size_t pos, size_t num_elems){
	if(dll == NULL || num_elems == 0){
		return 1;
	}

	if(vdll_seek(dll, pos) != 0){
		return 2;
	}

	if(vdll_wind(dll, num_elems - 1) != 0 || vdll_rewind(dll, num_elems - 1) != 0){
		return 2;
	}

	Vdll_node *current, *prev, *next;
	for(size_t i = 0; i < num_elems; i++){
		current = dll->ptr;
		prev = current->prev;
		next = current->next;

		vdll_node_destroy(current);
		if(prev == NULL && next == NULL){
			dll->ptr = NULL;
		} else if(prev == NULL){
			dll->ptr = next;
			dll->ptr->prev = NULL;
		} else if(next == NULL){
			dll->ptr = prev;
			dll->ptr->next = NULL;
		} else {
			prev->next = next;
			next->prev = prev;
			dll->ptr = prev;
		}

		dll->pos--;
		dll->cap--;
	}

	return 0;
}

int vdll_grow(Vdll *dll, size_t num_elems){
	if(dll == 0 || num_elems == 0){
		return 1;
	}

	size_t end = 0;
	if(dll->cap > 1){
		end = dll->cap - 1;
	}
	if(vdll_insert(dll, end, num_elems) != 0){
		return 2;
	}

	return 0;
}

int vdll_shrink(Vdll *dll, size_t num_elems){
	if(dll == 0 || num_elems == 0){
		return 1;
	}

	if(vdll_delete(dll, dll->cap - num_elems, num_elems) != 0){
		return 2;
	}

	return 0;
}
