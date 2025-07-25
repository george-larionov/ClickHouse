#!/usr/bin/env bash
# Tags: long, no-fasttest

CURDIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=../shell_config.sh
. "$CURDIR"/../shell_config.sh

export CURR_DATABASE="test_lazy_01294_concurrent_${CLICKHOUSE_DATABASE}"


function recreate_lazy_func1()
{
    $CLICKHOUSE_CLIENT -q "
        CREATE TABLE $CURR_DATABASE.log (a UInt64, b UInt64) ENGINE = Log;
    ";

    local TIMELIMIT=$((SECONDS+TIMEOUT))
    while [ $SECONDS -lt "$TIMELIMIT" ]
    do
        $CLICKHOUSE_CLIENT -q "
            DETACH TABLE $CURR_DATABASE.log;
        ";

        $CLICKHOUSE_CLIENT -q "
            ATTACH TABLE $CURR_DATABASE.log;
        ";
    done
}

function recreate_lazy_func2()
{
    local TIMELIMIT=$((SECONDS+TIMEOUT))
    while [ $SECONDS -lt "$TIMELIMIT" ]
    do
        $CLICKHOUSE_CLIENT -q "
            CREATE TABLE $CURR_DATABASE.tlog (a UInt64, b UInt64) ENGINE = TinyLog;
        ";

        $CLICKHOUSE_CLIENT -q "
            DROP TABLE $CURR_DATABASE.tlog;
        ";
    done
}

function recreate_lazy_func3()
{
    $CLICKHOUSE_CLIENT -q "
        CREATE TABLE $CURR_DATABASE.slog (a UInt64, b UInt64) ENGINE = StripeLog;
    ";

    local TIMELIMIT=$((SECONDS+TIMEOUT))
    while [ $SECONDS -lt "$TIMELIMIT" ]
    do
        $CLICKHOUSE_CLIENT -q "
            ATTACH TABLE $CURR_DATABASE.slog;
        ";

        $CLICKHOUSE_CLIENT -q "
            DETACH TABLE $CURR_DATABASE.slog;
        ";
    done
}

function recreate_lazy_func4()
{
    local TIMELIMIT=$((SECONDS+TIMEOUT))
    while [ $SECONDS -lt "$TIMELIMIT" ]
    do
        $CLICKHOUSE_CLIENT -q "
            CREATE TABLE $CURR_DATABASE.tlog2 (a UInt64, b UInt64) ENGINE = TinyLog;
        ";

        $CLICKHOUSE_CLIENT -q "
            DROP TABLE $CURR_DATABASE.tlog2;
        ";
    done
}

function test_func()
{
    local TIMELIMIT=$((SECONDS+TIMEOUT))
    while [ $SECONDS -lt "$TIMELIMIT" ]
    do
        for table in log tlog slog tlog2; do
            $CLICKHOUSE_CLIENT -q "SYSTEM STOP TTL MERGES $CURR_DATABASE.$table" >& /dev/null
        done
    done
}


${CLICKHOUSE_CLIENT} -q "
    DROP DATABASE IF EXISTS $CURR_DATABASE;
    CREATE DATABASE $CURR_DATABASE ENGINE = Lazy(1);
"


TIMEOUT=20

recreate_lazy_func1 2> /dev/null &
recreate_lazy_func2 2> /dev/null &
recreate_lazy_func3 2> /dev/null &
recreate_lazy_func4 2> /dev/null &
test_func 2> /dev/null &

wait
sleep 1

for table in log tlog slog tlog2; do
    $CLICKHOUSE_CLIENT -q "SYSTEM STOP TTL MERGES $CURR_DATABASE.$table" >& /dev/null
  ${CLICKHOUSE_CLIENT} -q "ATTACH TABLE $CURR_DATABASE.$table;" 2>/dev/null
done

${CLICKHOUSE_CLIENT} -q "DROP DATABASE $CURR_DATABASE"

echo "Test OK"

