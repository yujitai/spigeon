#g++ -c url_snprintf.cpp url_snprintf.h
#g++ -c log.cpp url_snprintf.cpp url_snprintf.h log.h store_error.h -I./gtest/include -I./libevent/include
#g++ -o test_log -lpthread test_log.cpp log.o url_snprintf.o ./gtest/lib/gtest.a -I./ -I./libevent/include -I./gtest/include \
#        -L./libevent/lib -Wl,-Bstatic -levent -levent_core -Wl,-Bdynamic -lrt 
make
log_to_stderr=1 ./test_log
