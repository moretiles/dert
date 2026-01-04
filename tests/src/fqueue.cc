namespace {
TEST(fqueue, simple) {
    Fqueue *in = fqueue_create(999, "tests/fqueue/fqueue_in.txt", "r");
    Fqueue *out = fqueue_create(999, "tests/fqueue/fqueue_out.txt", "w");
    ASSERT_NE(nullptr, in);
    ASSERT_NE(nullptr, out);

    // Enqueue / Dequeue test
    char dequeue_text[] = "testing dequeue\n";
    size_t dequeue_text_len = strlen(dequeue_text);
    char dequeue_text_check[999] = "";
    ASSERT_EQ(0, fqueue_enqueue(in, dequeue_text, dequeue_text_len));
    ASSERT_EQ(0, fqueue_dequeue(in, dequeue_text_check, fqueue_used(in)));
    ASSERT_STREQ(dequeue_text, dequeue_text_check);
    // now that all text content in the fqueue in has been consumed fold down
    ASSERT_EQ(fqueue_prev(in), dequeue_text_len);
    ASSERT_EQ(fqueue_used(in), 0);
    ASSERT_GT(in->readCursor, 0);
    ASSERT_GT(in->writeCursor, 0);
    ASSERT_EQ(0, fqueue_fold_down(in));
    ASSERT_EQ(0, in->readCursor);
    ASSERT_EQ(0, in->writeCursor);

    // Exchange / Rewind_Read_Cursor test
    char exchange_text[] = "testing exchange\n";
    size_t exchange_text_len = strlen(exchange_text);
    char exchange_text_check[999] = "";
    ASSERT_EQ(0, fqueue_enqueue(in, exchange_text, exchange_text_len));
    ASSERT_EQ(0, fqueue_exchange(in, out, fqueue_used(in)));
    ASSERT_EQ(0, fqueue_dequeue(out, exchange_text_check, fqueue_used(out)));
    ASSERT_STREQ(exchange_text, exchange_text_check);
    ASSERT_EQ(0, fqueue_rewind_read_cursor(in, exchange_text_len));
    ASSERT_EQ(0, fqueue_exchange(in, out, fqueue_used(in)));
    ASSERT_EQ(0, fqueue_dequeue(out, exchange_text_check, fqueue_used(out)));
    ASSERT_EQ(0, fqueue_fold_down(in));
    ASSERT_EQ(0, fqueue_fold_down(out));

    // Rewind_Write_Cursor test
    char rewind_text[] = "123456";
    size_t rewind_text_len = 6;
    char rewind_text_partial[] = "123";
    char rewind_text_check[999] = "";
    size_t rewind_text_partial_len = 3;
    ASSERT_EQ(0, fqueue_enqueue(in, rewind_text, rewind_text_len));
    ASSERT_EQ(0, fqueue_rewind_write_cursor(in, rewind_text_len - rewind_text_partial_len));
    ASSERT_EQ(0, fqueue_dequeue(in, rewind_text_check, rewind_text_partial_len));
    ASSERT_STREQ(rewind_text_partial, rewind_text_check);
    ASSERT_EQ(0, fqueue_fold_down(in));

    // Fenqueue / Fdequeue test
    size_t fenqueue_text_len = 18;
    ASSERT_EQ(0, fqueue_fenqueue(in, fenqueue_text_len));
    ASSERT_EQ(0, fqueue_exchange(in, out, fenqueue_text_len));
    ASSERT_EQ(0, fqueue_fdequeue(out, fqueue_used(out)));

    fqueue_destroy(in);
    fqueue_destroy(out);
}
}
