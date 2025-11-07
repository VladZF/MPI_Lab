#!/bin/bash

TASKS_DIR="Tasks"
BUILD_DIR="Build"
RESULTS_DIR="Results"

MATRIX_ROWS=16384
MATRIX_COLS=16384
PROCESSES_TO_RUN=(1 2 4 8)


clear
echo "================================================="
echo "   Запуск Задачи 2: Умножение матрицы на вектор"
echo "================================================="
echo "Пожалуйста, выберите метод разбиения:"
echo "1. По строкам (Row-wise)"
echo "2. По столбцам (Column-wise) - (еще не реализовано)"
echo "3. По блокам (Block-wise) - (еще не реализовано)"
echo ""
read -p "Ваш выбор [1, 2]: " choice
choice=${choice:-1}

case $choice in
    1)
        METHOD_NAME="rows"
        ;;
    2)
        METHOD_NAME="cols"
        ;;
    3)
        echo "Метод 'по блокам' еще не реализован."
        exit 1
        ;;
    *)
        echo "Неверный выбор."
        exit 1
        ;;
esac

SOURCE_FILE_NAME="task2_matvec_${METHOD_NAME}"
SOURCE_FILE_PATH="$TASKS_DIR/${SOURCE_FILE_NAME}.c"
EXECUTABLE_PATH="$BUILD_DIR/$SOURCE_FILE_NAME"
RESULTS_FILE="$RESULTS_DIR/${SOURCE_FILE_NAME}.csv"

if [ ! -f "$SOURCE_FILE_PATH" ]; then
    echo "ОШИБКА: Исходный файл не найден: $SOURCE_FILE_PATH"
    exit 1
fi

echo ""
echo "--> Компиляция программы: $SOURCE_FILE_PATH"
mkdir -p $BUILD_DIR
mpicc -O3 $SOURCE_FILE_PATH -o $EXECUTABLE_PATH
if [ $? -ne 0 ]; then
    echo "--> ОШИБКА КОМПИЛЯЦИИ."
    exit 1
else
    echo "--> Компиляция прошла успешно."
fi
echo ""

mkdir -p $RESULTS_DIR
echo "--> Создание файла для результатов: $RESULTS_FILE"
echo "Processes,Matrix_Rows,Matrix_Cols,Time_Seconds" > $RESULTS_FILE
echo ""

echo "--> Умножение матрицы ${MATRIX_ROWS}x${MATRIX_COLS} методом '${METHOD_NAME}'..."
echo "========================================================"

for N_PROCS in "${PROCESSES_TO_RUN[@]}"
do
    if [[ "$METHOD_NAME" == "rows" ]] && [ $(($MATRIX_ROWS % $N_PROCS)) -ne 0 ]; then
        echo "--> ПРОПУСК: $MATRIX_ROWS строк не делится на $N_PROCS процессов."
        continue
    elif [[ "$METHOD_NAME" == "cols" ]] && [ $(($MATRIX_COLS % $N_PROCS)) -ne 0 ]; then
        echo "--> ПРОПУСК: $MATRIX_COLS столбцов не делится на $N_PROCS процессов."
        continue
    fi
    
    echo -n "--> ВЫПОЛНЕНИЕ НА $N_PROCS ПРОЦЕССАХ... "
    mpiexec -np $N_PROCS $EXECUTABLE_PATH $MATRIX_ROWS $MATRIX_COLS >> $RESULTS_FILE
    echo "[ЗАВЕРШЕНО]"
    echo "--------------------------------------------------------"
done

echo "--> Все запуски завершены. Результаты записаны в $RESULTS_FILE"