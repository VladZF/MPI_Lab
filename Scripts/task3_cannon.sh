#!/bin/bash

TASKS_DIR="Tasks"
BUILD_DIR="Build"
RESULTS_DIR="Results"
SOURCE_FILE_NAME="task3_cannon"

MATRIX_SIZES=(960 1200 1440 1680 1920)
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
echo "--> Создание/очистка файла для результатов: $RESULTS_FILE"
echo "Processes,Matrix_Size,Time_Seconds" > $RESULTS_FILE

echo "--> Запуск тестов для алгоритма Кэннона..."
echo "========================================================"

for MATRIX_SIZE in "${MATRIX_SIZES[@]}"
do
    echo "--> Тестирование матрицы размером ${MATRIX_SIZE}x${MATRIX_SIZE}..."

    for N_PROCS in "${PROCESSES_TO_RUN[@]}"
    do
        GRID_DIM=$(echo "sqrt($N_PROCS)" | bc)
        if [ $((GRID_DIM * GRID_DIM)) -ne $N_PROCS ]; then
            echo "--> ПРОПУСК: Число процессов ($N_PROCS) не является полным квадратом."
            continue
        fi

        if [ $((MATRIX_SIZE % GRID_DIM)) -ne 0 ]; then
            echo "--> ПРОПУСК: Размер матрицы ($MATRIX_SIZE) не делится на размер сетки ($GRID_DIM)."
            continue
        fi

        echo -n "--> ВЫПОЛНЕНИЕ НА $N_PROCS ПРОЦЕССАХ... "
        mpiexec -np $N_PROCS --use-hwthread-cpus $EXECUTABLE_PATH $MATRIX_SIZE >> $RESULTS_FILE
        echo "[ЗАВЕРШЕНО]"
    done
    echo "--------------------------------------------------------"
done

echo "--> Все запуски завершены. Результаты записаны в $RESULTS_FILE"
