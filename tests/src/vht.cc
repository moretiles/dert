namespace {
TEST(vht, simple) {
    Vht *table = vht_create(sizeof(long), sizeof(char));
    ASSERT_NE(nullptr, table);
    ASSERT_EQ(0, vht_len(table));

#define TEST_VHT_ARRAY_LEN (257)
    long keys[TEST_VHT_ARRAY_LEN];
    char vals[TEST_VHT_ARRAY_LEN];
    char ptrs[TEST_VHT_ARRAY_LEN];
    size_t i;
    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        keys[i] = rand();
        vals[i] = rand();
    }
    ASSERT_EQ(nullptr, vht_get_direct(table, &(keys[0])));

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        ASSERT_EQ(0, vht_set(table, &(keys[i]), &(vals[i])));
    }
    ASSERT_EQ(TEST_VHT_ARRAY_LEN, vht_len(table));

    for(i = 0; i < TEST_VHT_ARRAY_LEN; i++) {
        ASSERT_EQ(0, vht_get(table, &(keys[i]), &(ptrs[i])));
        ASSERT_EQ(ptrs[i], vals[i]);
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

    ASSERT_EQ(0, vht_iterate_start(table, &iterator));
    while((res = vht_iterate_next(table, &iterator, &dest_key, &dest_val)) != ENODATA) {
        ASSERT_EQ(0, res);

        vht_sum += dest_val;
    }
    ASSERT_EQ(vals_sum, vht_sum);

    vht_destroy(table);
}
}
