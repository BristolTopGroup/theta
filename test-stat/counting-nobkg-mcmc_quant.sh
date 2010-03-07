#!/bin/bash


fail()
{
   echo "FAILED: $*";
   exit 1;
}

pass()
{
    echo "PASSED $*";
}

exec_theta()
{
    ../bin/theta -q $*
    [ "$?" -gt 0 ] && fail "theta returned error"
}


quantiles=( 0.5 0.16 0.84 0.95 )
for Theta in 5.0 10000.0; do
    sed "s/__THETA__/${Theta}/g" counting-nobkg.cfg.tpl > counting-nobkg.cfg
    exec_theta counting-nobkg-mcmc_quant.cfg
    events_total=50
    events_low=0
    events_hi=0
    for ievent in `seq ${events_total}`; do
        n_events=$(sqlite3 counting-nobkg-mcmc.db "select n_events_o from writer where eventid=${ievent};")
        theta_q_estimated=( $(sqlite3 -separator " " counting-nobkg-mcmc.db "select quant05000, quant01600, quant08400, quant09500 from mcmc_quant where eventid=${ievent};") )
        event_low=0
        event_hi=0
        for i in `seq 0 3`; do
            true_q=$(./true_q ${n_events} ${theta_q_estimated[i]})
#            echo "wanted quantile ${quantiles[i]}, got ${true_q}"
            error=$(echo "error=${quantiles[i]} - ${true_q};\
                          if(error<-0.015){ print \"low\";}else if(error>0.015){print \"hi\"} else {print \"ok\";}" | bc -l);
            if [ "$error" = "low" ]; then
                event_low=1
            fi
            if [ "$error" = "hi" ]; then
                event_hi=1
            fi
        done
        if [ $event_low -gt 0 ]; then
            events_low=$[$events_low + 1];
        fi
        if [ $event_hi -gt 0 ]; then
            events_hi=$[$events_hi + 1];
        fi
    done
    if test "( $events_low -lt 15 ) -a ( $events_hi -lt 15 )"; then
        pass $0 "Theta = " ${Theta} events with estimates too low: ${events_low}, too high: ${events_hi}
    else
        fail $0 "Theta = " ${Theta} events with estimates too low: ${events_low}, too high: ${events_hi}
    fi
done
