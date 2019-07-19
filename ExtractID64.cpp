#define RAPIDJSON_SIMD
#define RAPIDJSON_SSE42

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"
#include <iostream>
#include "zstr.hpp"
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
//#include <charconv>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>

using namespace std;
using namespace std::chrono;
using namespace rapidjson;

unsigned long lookup[] = {
	0,1,2,3,4,5,6,7,8,9,
	0,10,20,30,40,50,60,70,80,90,
	0,100,200,300,400,500,600,700,800,900,
	0,1000,2000,3000,4000,5000,6000,7000,8000,9000,
	0,10000,20000,30000,40000,50000,60000,70000,80000,90000,
	0,100000,200000,300000,400000,500000,600000,700000,800000,900000,
	0,1000000,2000000,3000000,4000000,5000000,6000000,7000000,8000000,9000000,
	0,10000000,20000000,30000000,40000000,50000000,60000000,70000000,80000000,90000000,
	0,100000000,200000000,300000000,400000000,500000000,600000000,700000000,800000000,900000000,
	0,1000000000,2000000000,3000000000,4000000000,5000000000,6000000000,7000000000,8000000000,9000000000,
	0,10000000000,20000000000,30000000000,40000000000,50000000000,60000000000,70000000000,80000000000,90000000000,
	0,100000000000,200000000000,300000000000,400000000000,500000000000,600000000000,700000000000,800000000000,900000000000,
	0,1000000000000,2000000000000,3000000000000,4000000000000,5000000000000,6000000000000,7000000000000,8000000000000,9000000000000,
	0,10000000000000,20000000000000,30000000000000,40000000000000,50000000000000,60000000000000,70000000000000,80000000000000,90000000000000,
	0,100000000000000,200000000000000,300000000000000,400000000000000,500000000000000,600000000000000,700000000000000,800000000000000,900000000000000,
	0,1000000000000000,2000000000000000,3000000000000000,4000000000000000,5000000000000000,6000000000000000,7000000000000000,8000000000000000,9000000000000000,
	0,10000000000000000,20000000000000000,30000000000000000,40000000000000000,50000000000000000,60000000000000000,70000000000000000,80000000000000000,90000000000000000,
	0,100000000000000000,200000000000000000,300000000000000000,400000000000000000,500000000000000000,600000000000000000,700000000000000000,800000000000000000,900000000000000000,
	0,1000000000000000000,2000000000000000000,3000000000000000000,4000000000000000000,5000000000000000000,6000000000000000000,7000000000000000000,8000000000000000000,900000000000000000
};

static inline uint64_t parse_16_digits_unrolled(const char *chars) {
  // this actually computes *16* values so we are being wasteful.
  const __m128i ascii0 = _mm_set1_epi8('0');
  const __m128i mul_1_10 =
      _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
  const __m128i mul_1_100 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
  const __m128i mul_1_10000 =
      _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);
  const __m128i mul_1_100000000 =
      _mm_setr_epi32(1, 1, 100000000, 100000000);
  const __m128i input = _mm_sub_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i *>(chars)), ascii0);
  const __m128i t1 = _mm_maddubs_epi16(input, mul_1_10);
  const __m128i t2 = _mm_shuffle_epi32(_mm_madd_epi16(t1, mul_1_100), _MM_SHUFFLE(1, 0, 3, 2));
  const __m128i t3 = _mm_packus_epi32(t2, t2);
  const __m128i t4 = _mm_madd_epi16(t3, mul_1_10000);
  const __m128i t5 = _mm_shuffle_epi32(t4, _MM_SHUFFLE(3, 1, 2, 0));
  const __m128i t6 = _mm_mul_epi32(t5, mul_1_100000000);
  const __m128i t7 = _mm_add_epi64(t6, _mm_shuffle_epi32(t6, _MM_SHUFFLE(1, 0, 3, 2)));
  return _mm_cvtsi128_si64(t7);
}

static inline unsigned long naive_lookup(const char *p,int length) {
    unsigned long x = 0;
	unsigned long count = 0;

    --length;

    while (length >= 0) {
		x += lookup[(*(p+length) - '0') + count];
        --length;
        count += 10;
    }
    return x;
}

static inline unsigned long naive(const char *p) {
    unsigned long x = 0;

    while (*p) {
		x = (x * 10) + (*p - '0');
        ++p;
    }
    return x;
}

static inline unsigned digit_value (char c)
{
   return unsigned(c - '0');
}

static inline uint64_t extract_uint64 (char const **read_ptr)
{
   char const *p = *read_ptr;
   uint64_t n = digit_value(*p);
   unsigned d;

   while ((d = digit_value(*++p)) <= 9)
   {
      n = n * 10 + d;
   }

   *read_ptr = p;

   return n;
}


class Handler {
    protected:
        unsigned long int lines;
        unsigned long int depth;
        bool id64FieldNext;
        high_resolution_clock::time_point t1,t2;
    public:
        Handler() {
            this->lines = 1;
            this->depth = 0;
            this->id64FieldNext = false;
            this->t1 = high_resolution_clock::now();
        }
        ~Handler() {
        }
        bool Key(const char* str, SizeType length, bool copy) {
            (void)copy; (void)length;
            if (this->depth == 2 && strncmp(str,"id64",4) == 0) {
                this->id64FieldNext = true;
            } else {
                this->id64FieldNext = false;
            }
            return true;
        }
        bool Null() {
            return true;
        }
        bool Bool(bool b) {
            (void) b;
            return true;
        }
        bool Int(int i) {
            if (this->id64FieldNext) {
                printf("%lX\n", (unsigned long) i);
            }
            return true;
        }
        bool Uint(unsigned u) {
            if (this->id64FieldNext) {
                printf("%lX\n", (unsigned long) u);
            }
            return true;
        }
        bool Int64(int64_t i) {
            if (this->id64FieldNext) {
                printf("%lX\n", (unsigned long) i);
            }
            return true;
        }
        bool Uint64(uint64_t u) {
            if (this->id64FieldNext) {
                printf("%lX\n", u);
            }
            return true;
        }
        bool Double(double d) {
            (void) d;
            return true;
        }
        bool RawNumber(const char* str, SizeType length, bool copy) {
            (void) length; (void) copy;
            if (this->id64FieldNext) {
                unsigned long int lastId64;
                // lastId64 = strtoul(str,NULL,0);
                // lastId64 = naive(str);
                lastId64 = naive_lookup(str,length);
                // lastId64 = parse_16_digits_unrolled(str);
                // lastId64 = extract_uint64(&str);
                // std::from_chars(str,str + length,lastId64);

                printf("%lX\n", lastId64);
            }
            return true;
        }
        bool String(const char* str, SizeType length, bool copy) {
            (void) str; (void) length; (void) copy;
            return true;
        }
        bool StartObject() {
            this->depth++;
            return true;
        }
        bool StartArray() {
            this->depth++;
            return true;
        }
        bool EndObject(SizeType memberCount) {
            (void)memberCount;
            this->depth--;
            if (this->depth == 1) {
                this->lines++;
                if (this->lines % 5000000 == 0) {
                    this->t2 = high_resolution_clock::now();
                    duration<double> total_time_span = duration_cast<duration<double>>(t2 - t1);
                    cerr << "Read line " << this->lines << " in " << total_time_span.count() << " seconds at " << (this->lines / total_time_span.count()) << " lines/second" << endl;
                }
            }
            return true;
        }
        bool EndArray(SizeType elementCount) {
            (void)elementCount;
            this->depth--;
            return true;
        }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file to parse>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    string filename = argv[1];

    struct stat buffer;
    if (stat (filename.c_str(), &buffer) != 0) {
        cerr << "File " << filename << " does not exist." << endl;
        exit(1);
    }

    Reader reader;
    zstr::ifstream* ifs;
    IStreamWrapper* stream;
    Handler handler;

    ifs = new zstr::ifstream(filename);
    stream = new IStreamWrapper(*ifs);
    reader.IterativeParseInit();

    //reader.Parse<kParseNumbersAsStringsFlag>(*stream, handler);
    reader.Parse<kParseDefaultFlags | kParseNumbersAsStringsFlag>(*stream, handler);
    if (reader.HasParseError()) {
        cerr << "Parse error in " <<
            filename << " [" <<
            GetParseError_En(reader.GetParseErrorCode()) <<
            "] offset [" <<
            reader.GetErrorOffset() <<
            endl;
        throw "Parse error in systems";
    }

    delete stream;
    delete ifs;
}
