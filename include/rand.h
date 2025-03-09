#define MAX 1000
class Rand {
   public:
    Rand(int seed = 1) {
        next = seed;
    }

    int random() {
        return (int)((next = next * 1103515245 + 12345) % ((MAX + 1)));
    }

   private:
    int next;
};
