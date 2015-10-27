#include "util/config.h"
#include "inc/env.h"
#include "db/black_hole_engine.h"

using namespace store;

int main() {
  Config *conf = new Config;

  Engine *engine = new BlackHoleEngine;
  
  
  if (Env::init(conf, engine) == ENV_ERROR) {
    printf("env init failed");
    return 1;
  }
  Env::start();
  Env::cleanup();

  delete engine;
  delete conf;
}
