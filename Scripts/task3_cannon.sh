#!/bin/bash

# --- Конфигурация ---
TASKS_DIR="Tasks"
BUILD_DIR="Build"
RESULTS_DIR="Results"
SOURCE_FILE_NAME="task3_cannon"

MATRIX_SIZE=1080
PROCESSES_TO_RUN=(1 4 9)

SOURCE_FILE_PATH="$TASKS_DIR/${SOURCE_FILE_NAME}.c"
EXECUTABLE_PATH="$BUILD_DIR/$SOURCE_FILE_NAME"
RESULTS_FILE="$RESULTS_DIR/${SOURCE_FILE_NAME}.csv"

if [ ! -f "$SOURCE_FILE_PATH" ]; then
    echo "ОШИБКА: Исходный файл не найден: $SOURCE_FILE_PATH"
    exit 1
fi

echo "--> Компиляция программы: $SOURCE_FILE_PATH"
mkdir -p $BUILD_DIR
mpicc -O3 $SOURCE_FILE_PATH -o $EXECUTABLE_PATH -lm
if [ $? -ne 0 ]; then echo "--> ОШИБКА КОМПИЛЯЦИИ."; exit 1; fi
echo "--> Компиляция прошла успешно."
echo ""

mkdir -p $RESULTS_DIR
echo "--> Создание файла для результатов: $RESULTS_FILE"
echo "Processes,Matrix_Size,Time_Seconds" > $RESULTS_FILE

echo "--> Умножение матрицы ${MATRIX_SIZE}x${MATRIX_SIZE} по алгоритму Кэннона..."
echo "========================================================"

for N_PROCS in "${PROCESSES_TO_RUN[@]}"
do
    echo -n "--> ВЫПОЛНЕНИЕ НА $N_PROCS ПРОЦЕССАХ... "
    mpiexec -np $N_PROCS --use-hwthread-cpus $EXECUTABLE_PATH $MATRIX_SIZE >> $RESULTS_FILE
    echo "[ЗАВЕРШЕНО]"
done

echo "--------------------------------------------------------"
echo "--> Все запуски завершены. Результаты записаны в $RESULTS_FILE"
