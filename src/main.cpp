#include "attack_tables.h"
#include "uci.h"
#include "zobrist.h"


int main() {
    AttackTables::init();
    Zobrist::init();
    UCI uci;
    uci.run();
}
