#include <vdll.h>
#include <vdll_priv.h>
#include <pointerarith.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

Vdll_node *vdll_node_create(size_t elem_size, int (*init)(void *arg)){
	if(elem_size == 0){
		return NULL;
	}

	Vdll_node *ret = malloc(elem_size + sizeof(Vdll_node));
	if(ret == NULL){
		return NULL;
	}

	// pointer addition in this case scales by 1
	ret->data = pointer_literal_addition(ret, sizeof(Vdll_node));
    memset(ret->data, 0, elem_size);
    if(init != NULL){
        init(ret->data);
    }

	ret->prev = NULL;
	ret->next = NULL;
	return ret;
}

int vdll_node_destroy(Vdll_node *node, size_t elem_size, int (*deinit)(void *arg)){
	if(node != NULL){
        if(deinit != NULL){
            deinit(node->data);
        }
        memset(node, 0, sizeof(Vdll_node) + elem_size);
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

Vdll *vdll_create(size_t elem_size, Vdll_functions *functions){
	Vdll *ret = malloc(sizeof(Vdll));
	if(ret == 0){
		return NULL;
	}

    if(vdll_init(ret, elem_size, functions) != 0){
        free(ret);
        return NULL;
    }
	return ret;
}

int vdll_init(Vdll *dll, size_t elem_size, Vdll_functions *functions){
    if(dll == NULL || elem_size == 0){
        return 1;
    }

	dll->ptr = NULL;
	dll->elem_size = elem_size;
	dll->pos = 0;
	dll->cap = 0;
	dll->functions = functions;
    return 0;
}

void vdll_deinit(Vdll *dll){
    if(dll == NULL){
        return;
    }

	if(dll->cap != 0){
		vdll_shrink(dll, dll->cap);
	}

    return;
}

void vdll_destroy(Vdll *dll){
	if(dll == NULL){
		return;
	}

    vdll_deinit(dll);

	free(dll);
	return;
}

int vdll_get(Vdll *dll, size_t pos, void *dest){
    void *src;
	if(dll == NULL || dest == NULL){
		return 1;
	}

    src = vdll_get_direct(dll, pos);
	if(memcpy(dest, src, dll->elem_size) != dest){
		return 3;
	}

	return 0;
}

void *vdll_get_direct(Vdll *dll, size_t pos){
    if(dll == NULL){
        return NULL;
    }

	if(vdll_seek(dll, pos) != 0){
		return NULL;
	}

    return dll->ptr->data;
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

size_t vdll_len(Vdll *dll){
	if(dll == NULL){
		return 0;
	} else {
		return dll->cap;
	}
}

int vdll_insert(Vdll *dll, size_t pos, size_t num_elems){
    return _vdll_insert(dll, pos, num_elems, false);
}

int _vdll_insert(Vdll *dll, size_t pos, size_t num_elems, bool after){
	if(dll == NULL || num_elems == 0){
		return 1;
	}

	if(vdll_seek(dll, pos) != 0){
		return 2;
	}

	for(size_t i = 0; i < num_elems; i++){
        int (*init)(void *arg) = NULL;
        if(dll->functions != NULL && dll->functions->init != NULL){
            init = dll->functions->init;
        }
        Vdll_node *new = vdll_node_create(dll->elem_size, init);

		if(new == NULL){
			if(i > 1){
				// deallocate everything added prior to failure
				vdll_delete(dll, pos + 1, i);
			}
			return 3;
		}

        // after controls whether we insert before or after element ptr references.
		if(dll->ptr == NULL){
			dll->ptr = new;
		} else if (after){
            new->prev = dll->ptr->prev;
            new->next = dll->ptr;
            if(dll->ptr->prev){
                dll->ptr->prev->next = new;
            }
            dll->ptr->prev = new;

            // make sure pos stays same, even with nodes being inserted
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

        int (*deinit)(void *arg) = NULL;
        if(dll->functions != NULL && dll->functions->deinit != NULL){
            deinit = dll->functions->deinit;
        }
        vdll_node_destroy(current, dll->elem_size, deinit);

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

        if(dll->pos != 0){
		    dll->pos--;
        }

        if(dll->cap != 0){
		    dll->cap--;
        }
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
	if(_vdll_insert(dll, end, num_elems, true) != 0){
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
