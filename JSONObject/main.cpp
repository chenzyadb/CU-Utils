#include <iostream>
#include "CuJSONObject.h"

int main()
{
    using namespace CU;
    // item test.
    {
        JSONItem item{};
        std::cout << (item.isNull() ? "true" : "false") << std::endl;
        std::cout << (item == nullptr ? "true" : "false") << std::endl;
        std::cout << item.toRaw() << std::endl;
        item = true;
        std::cout << item.toBoolean() << std::endl;
        item = INT_MAX;
        std::cout << item.toInt() << std::endl;
        item = INT64_MAX;
        std::cout << item.toLong() << std::endl;
        item = 3.14159;
        std::cout << item.toDouble() << std::endl;
        item = "Hello, World!";
        std::cout << item.toString() << std::endl;
    }

    // array test.
    {
        JSONArray array{};
        array = std::vector<bool>{true, false, false};
        std::cout << array.toString() << std::endl;
        array = std::vector<int>{1, 2, 3};
        std::cout << array.toString() << std::endl;
        array = std::vector<int64_t>{12345678987654321, 98765432123456789};
        std::cout << array.toString() << std::endl;
        array = std::vector<double>{3.14, 7.62, 5.56};
        std::cout << array.toString() << std::endl;
        array = std::vector<std::string>{"This", "is", "List"};
        std::cout << array.toString() << std::endl;
        array = JSONArray("[[[]], [[[], []], [[], []]], [[[], [], []], [[], [], []], [[], [], []]]]");
        std::cout << array.toString() << std::endl;

        array = JSONArray("[1, 2, 3, \"text\", null]");
        std::cout << array.toString() << std::endl;
        array.remove("text");
        array.remove(2);
        std::cout << array.toString() << std::endl;

        array.clear();
        array.add(true);
        array.add(false);
        array.add(INT_MAX);
        array.add(INT64_MAX);
        array.add(3.14);
        array.add("Hello, World!");
        array.add(JSONArray());
        array.add(JSONObject());
        std::cout << array.toString() << std::endl;

        array += JSONArray("[1, 2, 3, 4, 5, \"sodayo!\"]");
        std::cout << array.toString() << std::endl;

        auto merged_array = array + JSONArray("[[]]");
        std::cout << merged_array.toString() << std::endl;
    }

    // object test.
    {
        JSONObject object(
            "{\"boolean\": true, \"int\": 12345678, \"long\": 12345678987654321, \"double\": 3.141592, \n"
            "\"string\": \"Hello, World!\", \"array\": [true, 123, 123321123321123321, 5.56, \"test\", [], {}], \n"
            "\"object\": {\"val\": {}}, \"null\": null}"
        );
        std::cout << object.toFormatedString() << std::endl;
        std::cout << object["boolean"].toBoolean() << std::endl;
        std::cout << object["int"].toInt() << std::endl;
        std::cout << object["long"].toLong() << std::endl;
        std::cout << object["double"].toDouble() << std::endl;
        std::cout << object["string"].toString() << std::endl;
        std::cout << object["array"].toArray().toString() << std::endl;
        std::cout << object["object"].toObject().toString() << std::endl;

        object["newVal"] = JSONObject("{\"newObj\": \"test\"}");
        std::cout << object.toFormatedString() << std::endl;

        JSONObject object2("{\"newVal\": \"replaced\", \"23\": 456}");
        auto object3 = object2 + object;
        std::cout << object3.toFormatedString() << std::endl;

        for (const auto &pair : object3.toPairs()) {
            std::cout << "key: " << pair.key << " value: " << pair.value.toRaw() << std::endl;
        }

        if (object.at("null") == nullptr) {
            std::cout << "is null" << std::endl;
        } else {
            std::cout << "not null" << std::endl;
        }
    }

    // JSON PASS test.
    {
        // nativejson-benchmark by miloyip.
        constexpr char testJSONText[] =
            "[ \n"
            "    \"JSON Test Pattern pass1\",\n"
            "    {\"object with 1 member\":[\"array with 1 element\"]},\n"
            "    {},\n"
            "    [],\n"
            "    -42,\n"
            "    true,\n"
            "    false,\n"
            "    null,\n"
            "    {\n"
            "       \"integer\": 1234567890,\n"
            "        \"real\": -9876.543210,\n"
            "        \"e\": 0.123456789e-12,\n"
            "        \"E\": 1.234567890E+34,\n"
            "        \"\":  23456789012E66,\n"
            "        \"zero\": 0,\n"
            "        \"one\": 1,\n"
            "        \"space\": \" \",\n"
            "        \"quote\": \"\\\"\",\n"
            "        \"backslash\": \"\\\\\",\n"
            "        \"controls\": \"\\b\\f\\n\\r\\t\",\n"
            "        \"slash\": \"/ & \\/\",\n"
            "        \"alpha\": \"abcdefghijklmnopqrstuvwyz\",\n"
            "        \"ALPHA\": \"ABCDEFGHIJKLMNOPQRSTUVWYZ\",\n"
            "        \"digit\": \"0123456789\",\n"
            "        \"0123456789\": \"digit\",\n"
            "        \"special\": \"`1~!@#$%^&*()_+-={':[,]}|;.</>?\",\n"
            "        \"hex\": \"\\u0123\\u4567\\u89AB\\uCDEF\\uabcd\\uef4A\",\n"
            "        \"true\": true,\n"
            "        \"false\": false,\n"
            "        \"null\": null,\n"
            "        \"array\":[  ],\n"
            "        \"object\":{  },\n"
            "        \"address\": \"50 St. James Street\",\n"
            "        \"url\": \"http://www.JSON.org/\",\n"
            "        \"comment\": \"// /* <!-- --\",\n"
            "        \"# -- --> */\": \" \",\n"
            "        \" s p a c e d \" :[1,2 , 3\n"
            "\n"
            ",\n"
            "\n"
            "4 , 5        ,          6           ,7        ],\"compact\":[1,2,3,4,5,6,7],\n"
            "        \"jsontext\": \"{\\\"object with 1 member\\\":[\\\"array with 1 element\\\"]}\",\n"
            "        \"quotes\": \"&#34; \\u0022 %22 0x22 034 &#x22;\",\n"
            "        \"\\/\\\\\\\"\\uCAFE\\uBABE\\uAB98\\uFCDE\\ubcda\\uef4A\\b\\f\\n\\r\\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?\"\n"
            ": \"A key can be any string\"\n"
            "    },\n"
            "    0.5 ,98.6\n"
            ",\n"
            "99.44\n"
            ",\n"
            "\n"
            "1066,\n"
            "1e1,\n"
            "0.1e1,\n"
            "1e-1,\n"
            "1e00,2e+00,2e-00\n"
            ",\"rosebud\"]\n";
        JSONArray array(testJSONText);
        std::cout << array.toString() << std::endl;
        for (const auto &item : array) {
            std::cout << item.toRaw() << " hash: " << item.hash() << std::endl;
        }
        std::cout << array[8].toObject().toFormatedString() << std::endl;
    }

    return 0;
}
