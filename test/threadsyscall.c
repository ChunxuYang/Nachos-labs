#include "syscall.h"


int main() {
    SpaceId spcid;
    spcid = Exec("../test/halt");
    Join(spcid);
    Exit(100);
}