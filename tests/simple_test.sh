#!/bin/bash

if [ -z "$TEST_NUMBER" ]; then
  echo "No test number set. => test ignored"
  exit 1
fi

echo "Test #$TEST_NUMBER"
mkdir -p "logs/test_$TEST_NUMBER/"

INPUT_FILE="logs/test_$TEST_NUMBER/input_file"
OUTPUT_FILE="logs/test_$TEST_NUMBER/received_file"
LINK_SIM_LOG="logs/test_$TEST_NUMBER/link.log"
SENDER_LOG="logs/test_$TEST_NUMBER/sender.log"
RECEIVER_LOG="logs/test_$TEST_NUMBER/receiver.log"
VALGRIND_SENDER_LOG="logs/test_$TEST_NUMBER/valgrind_sender.log"
VALGRIND_RECEIVER_LOG="logs/test_$TEST_NUMBER/valgrind_receiver.log"

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

# On lance le simulateur de lien avec 10% de pertes et un délais de 50ms
./link_sim -p 1341 -P 2456 $LINK_SIM_ARGS &> "$LINK_SIM_LOG" &
link_pid=$!

# On lance le receiver et capture sa sortie standard
$valgrind_receiver ./receiver :: 2456 > "$OUTPUT_FILE" 2> "$RECEIVER_LOG" &
receiver_pid=$!

cleanup()
{
  kill -9 $receiver_pid
  kill -9 $link_pid
  exit 0
}
trap cleanup SIGINT  # Kill les process en arrière plan en cas de ^-C

# On démarre le transfert
if ! $valgrind_sender ./sender ::1 1341 < "$INPUT_FILE" 2> "$SENDER_LOG" ; then
  echo "Crash du sender!"
  # cat "$SENDER_LOG"
  err=1  # On enregistre l'erreur
fi

sleep 5 # On attend 5 seconde que le receiver finisse

if kill -0 $receiver_pid &> /dev/null ; then
  echo "Le receiver ne s'est pas arreté à la fin du transfert!"
  kill -9 $receiver_pid
  wait $receiver_pid 2>/dev/null
  err=1
else  # On teste la valeur de retour du receiver
  if ! wait $receiver_pid ; then
    echo "Crash du receiver!"
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
  echo "Le transfert a corrompu le fichier!"
  # echo "Diff binaire des deux fichiers: (attendu vs produit)"
  # diff -C 9 <(od -Ax -t x1z "$INPUT_FILE") <(od -Ax -t x1z "$OUTPUT_FILE")
  exit 1
else
  echo "Le transfert est réussi!"
  exit ${err:-0}  # En cas d'erreurs avant, on renvoie le code d'erreur
fi