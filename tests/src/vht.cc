namespace {
TEST(vht, simple) {
    Vht *table = vht_create(sizeof(long), sizeof(char));
    assert(table != NULL);
    assert(vht_len(table) == 0);

#define TEST_VHT_ARRAY_LEN (257)
    long keys[TEST_VHT_ARRAY_LEN];
    char vals[TEST_VHT_ARRAY_LEN];
    char ptrs[TEST_VHT_ARRAY_LEN];
    size_t i;
    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        keys[i] = rand();
        vals[i] = rand();
    }
    assert(vht_get_direct(table, &(keys[0])) == NULL);

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        assert(vht_set(table, &(keys[i]), &(vals[i])) == 0);
    }
    assert(vht_len(table) == TEST_VHT_ARRAY_LEN);

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        assert(vht_get(table, &(keys[i]), &(ptrs[i])) == 0);
        assert(ptrs[i] == vals[i]);
    }

    int64_t vals_sum = 0;
    long dest_key = 0;
    char dest_val = 0;
    Vht_iterator iterator;
    int64_t vht_sum = 0;
    int res;

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        vals_sum += vals[i];
    }

    assert(vht_iterate_start(table, &iterator) == 0);
    while((res = vht_iterate_next(table, &iterator, &dest_key, &dest_val)) != ENODATA) {
        assert(res == 0);

        vht_sum += dest_val;
    }
    assert(vals_sum == vht_sum);

    vht_destroy(table);
}
}
