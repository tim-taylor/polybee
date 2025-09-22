BEGIN {
    FS=","
}

{
    for (i=1; i<=NF; i++) {T+=$i}

}

END {
    print T
}
