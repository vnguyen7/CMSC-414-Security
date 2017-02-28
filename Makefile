CC = gcc
CFLAGS = -fno-stack-protector -Wall -Iutil -Iatm -Ibank -Irouter -I.

all: atm bank

atm : src/atm/atm-main.c src/atm/atm.c
	${CC} ${CFLAGS} src/atm/atm.c src/atm/atm-main.c -o atm -lcrypto

bank : src/bank/bank-main.c src/bank/bank.c src/util/hash_table.c src/util/list.c
	${CC} ${CFLAGS} src/bank/bank.c src/bank/bank-main.c src/util/hash_table.c src/util/list.c -o bank -lcrypto -lm

test : src/util/list.c src/util/list_example.c src/util/hash_table.c src/util/hash_table_example.c
	${CC} ${CFLAGS} src/util/list.c src/util/list_example.c -o list-test
	${CC} ${CFLAGS} src/util/list.c src/util/hash_table.c src/util/hash_table_example.c -o hash-table-test

clean:
	rm -f atm bank list-test hash-table-test
