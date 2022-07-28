#include <Arduino.h>
#include <unity.h>
#include <formatData.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void testFormatData() {
    volatile Measurements buff[] = {Measurements{0.0, 1.0}, Measurements{1.1, 1.2}, Measurements{2.2, 2.3}};
    String res = formatDataPoints(buff, 3, 1657733559010, 100);
                                        // 1657733558910
                                        // 1657733558710
    TEST_ASSERT_EQUAL_STRING(
        "test2 variable1=0.0000,variable2=1.0000 1657733558810\ntest2 variable1=1.1000,variable2=1.2000 1657733558910\ntest2 variable1=2.2000,variable2=2.3000 1657733559010\n",
        res.c_str()
    );
}

void testFromByteSlice() {
    double val1 = 1.23;
    const uint8_t* x = reinterpret_cast<const uint8_t*>(&val1);
    double a = fromByteSlice(x, 0, 8);
    TEST_ASSERT_EQUAL_DOUBLE(val1, a);
}

void testFromBytes() {
    uint8_t x[32] = {0};
    double val1 = 1.23;
    double val2 = 2.01;
    const uint8_t* x1 = reinterpret_cast<const uint8_t*>(&val1);
    const uint8_t* x2 = reinterpret_cast<const uint8_t*>(&val2);

    for (int i = 0; i <8;i++) {
        x[i] = x1[i];
        x[i+8] = x2[i];
    }
    x[16] = 69;
    
    Measurements a = fromBytes(x);
    TEST_ASSERT_EQUAL_DOUBLE(val1, a.v1);
    TEST_ASSERT_EQUAL_DOUBLE(val2, a.v2);
}

void testChecksum() {
    uint8_t d[] = {2, 5, 1};
    uint8_t res = checksum(d, 3);
    TEST_ASSERT_EQUAL_UINT8(6, res);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(testFormatData);
    RUN_TEST(testFromByteSlice);
    RUN_TEST(testFromBytes);
    RUN_TEST(testChecksum);
    UNITY_END();

}
void loop() {}
