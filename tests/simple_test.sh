#!/bin/bash

if [ -z "$TEST_NUMBER" ]; then
  echo "No test number set. => test ignored" >&2
  exit 1
fi

re='^[0-9]+$'
if ! [[ "$TEST_NUMBER" =~ $re ]]; then
  echo "TEST_NUMBER is not a number" >&2
  exit 1
fi

cleanup()
{
  if [ ! -z "$(jobs -pr)" ]; then
    kill $(jobs -pr)
  fi
  wait
  exit 0
}
trap cleanup INT TERM # Kill les process en arrière plan en cas de ^-C

sleep $TEST_NUMBER &
wait

rm -rf "logs/test_$TEST_NUMBER/"
mkdir -p "logs/test_$TEST_NUMBER/"

INFO_FILE="logs/test_$TEST_NUMBER/info_file"
DEBUG_FILE="logs/test_$TEST_NUMBER/debug_file"
INPUT_FILE="logs/test_$TEST_NUMBER/input_file"
OUTPUT_FILE="logs/test_$TEST_NUMBER/received_file"
LINK_SIM_LOG="logs/test_$TEST_NUMBER/link.log"
SENDER_LOG="logs/test_$TEST_NUMBER/sender.log"
RECEIVER_LOG="logs/test_$TEST_NUMBER/receiver.log"
VALGRIND_SENDER_LOG="logs/test_$TEST_NUMBER/valgrind_sender.log"
VALGRIND_RECEIVER_LOG="logs/test_$TEST_NUMBER/valgrind_receiver.log"

PORT1=`comm -23 <(seq 8000 8888 | sort) <(ss -Huan | awk '{print $4}' | cut -d':' -f2 | sort -u) | shuf | head -n 1`
PORT2=`comm -23 <(seq 8000 8888 | sort) <(ss -Huan | awk '{print $4}' | cut -d':' -f2 | sort -u) | shuf | head -n 1`

echo "Test #$TEST_NUMBER" >> "$INFO_FILE"
echo "PORT1=$PORT1, PORT2=$PORT2" >> "$DEBUG_FILE"

if [ -z "$FILE" ]; then
  if [ -z "$FILE_SIZE" ]; then
    $FILE_SIZE=0
  fi
  dd if=/dev/urandom of="$INPUT_FILE" bs=1 count=$FILE_SIZE &> /dev/null
else
  cp "$FILE" "$INPUT_FILE"
fi

valgrind_sender=""
valgrind_receiver=""
if [ ! -z "$VALGRIND" ] ; then
  valgrind_sender="valgrind --leak-check=full --log-file=$VALGRIND_SENDER_LOG"
  valgrind_receiver="valgrind --leak-check=full --log-file=$VALGRIND_RECEIVER_LOG"
fi

./link_sim -p $PORT1 -P $PORT2 $LINK_SIM_ARGS &> "$LINK_SIM_LOG" &
link_pid=$!

# On lance le receiver et capture sa sortie standard
$valgrind_receiver ./receiver :: $PORT2 > "$OUTPUT_FILE" 2> "$RECEIVER_LOG" &
receiver_pid=$!

echo "-------------------------------------------" >> "$DEBUG_FILE"
echo "netstat start" >> "$DEBUG_FILE"
echo "-------------------------------------------" >> "$DEBUG_FILE"
netstat -nlptu >> "$DEBUG_FILE" 2>/dev/null

# On démarre le transfert
sender_start_seconds=`date -u +"%s.%N"`
if ! $valgrind_sender ./sender ::1 $PORT1 < "$INPUT_FILE" 2> "$SENDER_LOG" ; then
  echo "Crash du sender!  <------------------------------" >> "$INFO_FILE"
  # cat "$SENDER_LOG"
  err=1  # On enregistre l'erreur
fi
sender_end_seconds=`date -u +"%s.%N"`

sleep 6 & # On attend 6 seconde que le receiver finisse
sleep_pid=$!
wait $sleep_pid

if kill -0 $receiver_pid &> /dev/null ; then
  echo "Le receiver ne s'est pas arreté à la fin du transfert!  <------------------------------" >> "$INFO_FILE"
  kill -9 $receiver_pid
  wait $receiver_pid 2>/dev/null
  err=1
else  # On teste la valeur de retour du receiver
  if ! wait $receiver_pid ; then
    echo "Crash du receiver!  <------------------------------" >> "$INFO_FILE"
    # cat "$RECEIVER_LOG"
    err=1
  fi
fi

# On arrête le simulateur de lien
if kill -0 $link_pid &> /dev/null ; then
  kill -9 $link_pid &> /dev/null
  wait $link_pid 2>/dev/null
fi

# On vérifie que le transfert s'est bien déroulé
if [[ `md5sum "$INPUT_FILE" | awk '{print $1}'` != `md5sum "$OUTPUT_FILE" | awk '{print $1}'` ]]; then
  echo "Le transfert a corrompu le fichier!  <------------------------------" >> "$INFO_FILE"
  # echo "Diff binaire des deux fichiers: (attendu vs produit)" >> "$INFO_FILE"
  # diff -C 9 <(od -Ax -t x1z "$INPUT_FILE") <(od -Ax -t x1z "$OUTPUT_FILE") >> "$INFO_FILE"
  exit 1
else
  echo "Le transfert est réussi!" >> "$INFO_FILE"
  date -u -d "0 $sender_end_seconds sec - $sender_start_seconds sec" +"%H:%M:%S.%N" >> "$INFO_FILE"
  exit ${err:-0}  # En cas d'erreurs avant, on renvoie le code d'erreur
fi