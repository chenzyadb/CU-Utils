// CuJSONObject by chenzyadb@github.com
// Based on C++17 STL (MSVC)

#if !defined(_CU_JSONOBJECT_)
#define _CU_JSONOBJECT_ 1

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif // defined(_MSC_VER)

#include <unordered_map>
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <exception>
#include <algorithm>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cstring>

namespace CU
{
	class JSONExcept : public std::exception
	{
		public:
			JSONExcept(const std::string &message) : message_(message) { }

			const char* what() const noexcept override
			{
				return message_.c_str();
			}

		private:
			const std::string message_;
	};

	struct _JSON_String
	{
		char stack_buffer[128];
		char* heap_block;
		size_t length;
		size_t capacity;

		_JSON_String() noexcept : stack_buffer(), heap_block(nullptr), length(0), capacity(sizeof(stack_buffer)) { }

		_JSON_String(const _JSON_String &other) noexcept :
			stack_buffer(),
			heap_block(nullptr),
			length(0),
			capacity(sizeof(stack_buffer))
		{
			append(other);
		}

		_JSON_String(_JSON_String &&other) noexcept :
			stack_buffer(),
			heap_block(other.heap_block),
			length(other.length),
			capacity(other.capacity)
		{
			std::memcpy(stack_buffer, other.stack_buffer, sizeof(stack_buffer));
			other.heap_block = nullptr;
		}

		_JSON_String(const char* src) noexcept :
			stack_buffer(),
			heap_block(nullptr),
			length(0),
			capacity(sizeof(stack_buffer))
		{
			append(src);
		}

		~_JSON_String() noexcept
		{
			if (heap_block != nullptr) {
				std::free(heap_block);
			}
		}

		void append(const _JSON_String &other) noexcept
		{
			auto new_len = length + other.length;
			if (new_len >= capacity) {
				resize(capacity + new_len);
			}
			std::memcpy((data() + length), other.data(), other.length);
			*(data() + new_len) = '\0';
			length = new_len;
		}

		void append(const char* src) noexcept
		{
			auto src_len = std::strlen(src);
			auto new_len = length + src_len;
			if (new_len >= capacity) {
				resize(capacity + new_len);
			}
			std::memcpy((data() + length), src, src_len);
			*(data() + new_len) = '\0';
			length = new_len;
		}

		void append(char ch) noexcept
		{
			if ((length + 1) >= capacity) {
				resize(capacity * 2);
			}
			*(data() + length) = ch;
			*(data() + length + 1) = '\0';
			length++;
		}

		void resize(size_t req_capacity) noexcept
		{
			if (req_capacity < capacity && length >= req_capacity) {
				shrink(req_capacity - 1);
			}
			if (req_capacity > sizeof(stack_buffer)) {
				auto new_block = reinterpret_cast<char*>(std::realloc(heap_block, req_capacity));
				if (new_block != nullptr) {
					if (heap_block == nullptr) {
						std::memcpy(new_block, stack_buffer, length);
					}
					*(new_block + length) = '\0';
					heap_block = new_block;
					capacity = req_capacity;
				}
			} else {
				capacity = sizeof(stack_buffer);
				if (heap_block != nullptr && length > 0) {
					std::memcpy(stack_buffer, heap_block, length);
					std::free(heap_block);
				}
				stack_buffer[length] = '\0';
			}
		}

		void shrink(size_t req_length) noexcept
		{
			if (req_length < length) {
				*(data() + req_length) = '\0';
				length = req_length;
			}
		}

		bool equals(const char* str) const noexcept
		{
			if (std::strlen(str) != length) {
				return false;
			}
			return (std::memcmp(data(), str, length) == 0);
		}

		char* data() noexcept
		{
			if (heap_block != nullptr) {
				return heap_block;
			}
			return stack_buffer;
		}

		const char* data() const noexcept
		{
			if (heap_block != nullptr) {
				return heap_block;
			}
			return stack_buffer;
		}

		void clear() noexcept
		{
			if (heap_block != nullptr) {
				std::free(heap_block);
				heap_block = nullptr;
			}
			stack_buffer[0] = '\0';
			length = 0;
			capacity = sizeof(stack_buffer);
		}
	};

	class JSONObject;
	class JSONArray;
	class JSONItem;

	namespace _JSON_Misc
	{
		inline char GetEscapeChar(const char &ch) noexcept
		{
			switch (ch) {
				case 'n':
					return '\n';
				case 'r':
					return '\r';
				case 't':
					return '\t';
				case 'b':
					return '\b';
				case 'f':
					return '\f';
				case 'a':
					return '\a';
				case 'v':
					return '\v';
				default:
					break;
			}
			return ch;
		}

		inline _JSON_String StringToJSONRaw(const std::string &str)
		{
			_JSON_String raw("\"");
			for (const auto &ch : str) {
				switch (ch) {
					case '\\':
						raw.append("\\\\");
						break;
					case '\"':
						raw.append("\\\"");
						break;
					case '\n':
						raw.append("\\n");
						break;
					case '\t':
						raw.append("\\t");
						break;
					case '\r':
						raw.append("\\r");
						break;
					case '\f':
						raw.append("\\f");
						break;
					case '\a':
						raw.append("\\a");
						break;
					case '\b':
						raw.append("\\b");
						break;
					case '\v':
						raw.append("\\v");
						break;
					case '/':
						raw.append("\\/");
						break;
					case '#':
						raw.append("\\#");
						break;
					default:
						raw.append(ch);
						break;
				}
			}
			raw.append('\"');
			return raw;
		}

		constexpr size_t npos = static_cast<size_t>(-1);

		inline size_t FindChar(const char* str, char ch, size_t start_pos = 0) noexcept
		{
			for (auto pos = start_pos; *(str + pos) != '\0'; pos++) {
				if (*(str + pos) == ch) {
					return pos;
				}
			}
			return npos;
		}
	}

	class JSONItem
	{
		public:
			enum class ItemType : uint8_t {ITEM_NULL, BOOLEAN, INTEGER, LONG, DOUBLE, STRING, ARRAY, OBJECT};

			typedef decltype(nullptr) ItemNull;
			typedef std::variant<ItemNull, bool, int, int64_t, double, std::string, JSONArray*, JSONObject*> ItemValue;

			JSONItem();
			JSONItem(const _JSON_String &raw);
			JSONItem(ItemNull _null);
			JSONItem(bool value);
			JSONItem(int value);
			JSONItem(int64_t value);
			JSONItem(double value);
			JSONItem(const char* value);
			JSONItem(const std::string &value);
			JSONItem(const JSONArray &value);
			JSONItem(const JSONObject &value);
			JSONItem(const JSONItem &other);
			JSONItem(JSONItem &&other) noexcept;
			~JSONItem();
			
			JSONItem &operator=(const JSONItem &other);
			JSONItem &operator=(JSONItem &&other) noexcept;
			bool operator==(const JSONItem &other) const;
			bool operator!=(const JSONItem &other) const;

			ItemType type() const;
			ItemValue value() const;
			ItemValue &&value_rv();
			void clear();
			size_t size() const;
			size_t hash() const;

			bool isNull() const;
			bool isBoolean() const;
			bool isInt() const;
			bool isLong() const;
			bool isDouble() const;
			bool isString() const;
			bool isArray() const;
			bool isObject() const;

			bool toBoolean() const;
			int toInt() const;
			int64_t toLong() const;
			double toDouble() const;
			std::string toString() const;
			JSONArray toArray() const;
			JSONObject toObject() const;
			std::string toRaw() const;
			
		private:
			ItemType type_;
			ItemValue value_;
	};

	class JSONArray
	{
		public:
			typedef std::vector<JSONItem>::iterator iterator;
			typedef std::vector<JSONItem>::const_iterator const_iterator;

			JSONArray();
			JSONArray(size_t init_size);
			JSONArray(size_t init_size, const JSONItem &init_value);
			JSONArray(iterator begin_iter, iterator end_iter);
			JSONArray(const std::string &jsonText);
			JSONArray(const char* jsonText);
			JSONArray(const std::vector<JSONItem> &data);
			JSONArray(const std::vector<bool> &list);
			JSONArray(const std::vector<int> &list);
			JSONArray(const std::vector<int64_t> &list);
			JSONArray(const std::vector<double> &list);
			JSONArray(const std::vector<std::string> &list);
			JSONArray(const std::vector<JSONArray> &list);
			JSONArray(const std::vector<JSONObject> &list);
			JSONArray(const JSONArray &other);
			JSONArray(JSONArray &&other) noexcept;
			~JSONArray();
			
			JSONArray &operator=(const JSONArray &other);
			JSONArray &operator=(JSONArray &&other) noexcept;
			JSONArray &operator+=(const JSONArray &other);
			JSONArray operator+(const JSONArray &other) const;
			bool operator==(const JSONArray &other) const;
			bool operator!=(const JSONArray &other) const;
			JSONItem &operator[](size_t pos);

			const JSONItem &at(size_t pos) const;
			iterator find(const JSONItem &item);
			void add(const JSONItem &item);
			void remove(const JSONItem &item);
			void resize(size_t new_size);
			void clear();
			size_t size() const;
			size_t hash() const;
			bool empty() const;
			const std::vector<JSONItem> &data() const;
			std::vector<JSONItem> &&data_rv();

			std::vector<bool> toListBoolean() const;
			std::vector<int> toListInt() const;
			std::vector<int64_t> toListLong() const;
			std::vector<double> toListDouble() const;
			std::vector<std::string> toListString() const;
			std::vector<JSONArray> toListArray() const;
			std::vector<JSONObject> toListObject() const;
			std::string toString() const;

			JSONItem &front();
			JSONItem &back();
			iterator begin();
			iterator end();
			const_iterator begin() const;
			const_iterator end() const;
			
		private:
			std::vector<JSONItem> data_;

			void Parse_Impl_(const char* json_text);
	};

	class JSONObject
	{
		public:
			JSONObject();
			JSONObject(const std::string &jsonText, bool enableComments = false);
			JSONObject(const char* jsonText, bool enableComments = false);
			JSONObject(const JSONObject &other);
			JSONObject(JSONObject &&other) noexcept;
			JSONObject(std::unordered_map<std::string, JSONItem> &&data, std::vector<std::string> &&order) noexcept;
			~JSONObject();

			JSONObject &operator=(const JSONObject &other);
			JSONObject &operator=(JSONObject &&other) noexcept;
			JSONObject &operator+=(const JSONObject &other);
			JSONObject operator+(const JSONObject &other) const;
			bool operator==(const JSONObject &other) const;
			bool operator!=(const JSONObject &other) const;
			JSONItem &operator[](const std::string &key);
			
			const JSONItem &at(const std::string &key) const;
			bool contains(const std::string &key) const;
			void add(const std::string &key, const JSONItem &value);
			void remove(const std::string &key);
			void clear();
			size_t size() const;
			size_t hash() const;
			bool empty() const;
			const std::unordered_map<std::string, JSONItem> &data() const;
			std::unordered_map<std::string, JSONItem> &&data_rv();
			const std::vector<std::string> &order() const;
			std::vector<std::string> &&order_rv();
			std::string toString() const;
			std::string toFormatedString() const;

			struct JSONPair
			{
				std::string key;
				JSONItem value;
			};
			std::vector<JSONPair> toPairs() const;

		private:
			std::unordered_map<std::string, JSONItem> data_;
			std::vector<std::string> order_;

			void Parse_Impl_(const char* json_text, bool enable_comments);
	};

	namespace JSON
	{
		JSONObject &Merge(JSONObject &dst, const JSONObject &src);
	}

	namespace JSONBinary
	{
		typedef uint32_t pos_t;
		typedef int8_t byte_t;

		constexpr pos_t npos = static_cast<pos_t>(-1);

		inline void* _Move_Ptr(void* ptr, pos_t pos) noexcept
		{
			return reinterpret_cast<void*>(reinterpret_cast<byte_t*>(ptr) + pos);
		}

		inline const void* _Move_Ptr(const void* ptr, pos_t pos) noexcept
		{
			return reinterpret_cast<const void*>(reinterpret_cast<const byte_t*>(ptr) + pos);
		}

		class _Binary_Container
		{
			public:
				_Binary_Container() noexcept : 
					block_(reinterpret_cast<byte_t*>(std::malloc(128))), 
					capacity_(128), 
					size_(0)
				{ }

				~_Binary_Container() noexcept
				{
					if (block_ != nullptr) {
						std::free(block_);
					}
				}

				void resize(pos_t req_capacity) noexcept
				{
					auto new_block = reinterpret_cast<byte_t*>(std::realloc(block_, req_capacity));
					if (new_block != nullptr) {
						block_ = new_block;
						capacity_ = req_capacity;
					}
					if (size_ > capacity_) {
						size_ = capacity_;
					}
				}

				void add(const void* data, pos_t data_size) noexcept
				{
					if ((size_ + data_size) > capacity_) {
						resize(size_ * 2 + data_size);
					}
					auto dst = _Move_Ptr(block_, size_);
					if (dst != nullptr && data != nullptr) {
						std::memcpy(dst, data, data_size);
						size_ += data_size;
					}
				}

				byte_t* data() noexcept
				{
					return block_;
				}

				byte_t* dump() noexcept
				{
					auto dump_data = block_;
					block_ = nullptr;
					capacity_ = 0;
					size_ = 0;
					return dump_data;
				}

				pos_t size() const noexcept
				{
					return size_;
				}

			private:
				byte_t* block_;
				pos_t capacity_;
				pos_t size_;
		};

		inline pos_t GetBinarySize(const byte_t* binary) noexcept
		{
			return *reinterpret_cast<const pos_t*>(binary);
		}

		inline void DeleteBinary(byte_t* binary) noexcept
		{
			if (binary != nullptr) {
				std::free(binary);
			}
		}

		byte_t* ArrayToBinary(const JSONArray &array);
		JSONArray BinaryToArray(const byte_t* binary);
		void SaveArray(const std::string &path, const JSONArray &array);
		JSONArray OpenArray(const std::string &path);

		byte_t* ObjectToBinary(const JSONObject &object);
		JSONObject BinaryToObject(const byte_t* binary);
		void SaveObject(const std::string &path, const JSONObject &object);
		JSONObject OpenObject(const std::string &path);
	}
}

namespace std
{
	template <>
	struct hash<CU::JSONItem>
	{
		size_t operator()(const CU::JSONItem &val) const
		{
			return val.hash();
		}
	};

	template <>
	struct hash<CU::JSONArray>
	{
		size_t operator()(const CU::JSONArray &val) const
		{
			return val.hash();
		}
	};

	template <>
	struct hash<CU::JSONObject>
	{
		size_t operator()(const CU::JSONObject &val) const
		{
			return val.hash();
		}
	};
}

#endif // !defined(_CU_JSONOBJECT_)
