#!/bin/bash

TASKS_DIR="Tasks"
BUILD_DIR="Build"
RESULTS_DIR="Results"

MATRIX_ROWS=16384
MATRIX_COLS=16384
PROCESSES_TO_RUN=(1 2 4 8)

run_tests_for_method() {
    METHOD_NAME=$1

    echo "========================================================"
    echo "--> Метод: '${METHOD_NAME}'"
    echo "========================================================"

    # Устанавливаем имена файлов
    SOURCE_FILE_NAME="task2_matvec_${METHOD_NAME}"
    SOURCE_FILE_PATH="$TASKS_DIR/${SOURCE_FILE_NAME}.c"
    EXECUTABLE_PATH="$BUILD_DIR/$SOURCE_FILE_NAME"
    RESULTS_FILE="$RESULTS_DIR/${SOURCE_FILE_NAME}.csv"

    if [ ! -f "$SOURCE_FILE_PATH" ]; then
        echo "--> ОШИБКА: Исходный файл не найден: $SOURCE_FILE_PATH"
        return 1
    fi

    echo "--> Компиляция программы: $SOURCE_FILE_PATH"
    mpicc -O3 $SOURCE_FILE_PATH -o $EXECUTABLE_PATH
    if [ $? -ne 0 ]; then
        echo "--> ОШИБКА КОМПИЛЯЦИИ."
        return 1
    fi
    echo "--> Компиляция прошла успешно."
    echo ""

    echo "--> Создание файла для результатов: $RESULTS_FILE"
    echo "Processes,Matrix_Rows,Matrix_Cols,Time_Seconds" > $RESULTS_FILE

    echo "--> Умножение матрицы ${MATRIX_ROWS}x${MATRIX_COLS}..."
    for N_PROCS in "${PROCESSES_TO_RUN[@]}"
    do
        echo -n "--> ВЫПОЛНЕНИЕ НА $N_PROCS ПРОЦЕССАХ... "
        mpiexec -np $N_PROCS --use-hwthread-cpus $EXECUTABLE_PATH $MATRIX_ROWS $MATRIX_COLS >> $RESULTS_FILE
        echo "[ЗАВЕРШЕНО]"
    done
    echo "--> Результаты записаны в $RESULTS_FILE"
}


mkdir -p $BUILD_DIR
mkdir -p $RESULTS_DIR

clear
echo "================================================="
echo "   Запуск Задачи 2: Умножение матрицы на вектор"
echo "================================================="
echo "Пожалуйста, выберите вариант запуска:"
echo "1. Только по строкам (Row-wise)"
echo "2. Только по столбцам (Column-wise)"
echo "3. Только по блокам (Block-wise)"
echo "4. ЗАПУСТИТЬ ВСЕ ВАРИАНТЫ ПОСЛЕДОВАТЕЛЬНО"
echo ""
echo "0. Выход"
echo "-------------------------------------------------"

read -p "Ваш выбор [1-4, 0]: " choice

case $choice in
    1)
        run_tests_for_method "rows"
        ;;
    2)
        run_tests_for_method "cols"
        ;;
    3)
        run_tests_for_method "blocks"
        ;;
    4)
        echo ""
        echo "--> ЗАПУСК ВСЕХ ВАРИАНТОВ..."
        run_tests_for_method "rows"
        echo ""
        run_tests_for_method "cols"
        echo ""
        run_tests_for_method "blocks"
        ;;
    0)
        echo "Выход."
        exit 0
        ;;
    *)
        echo "Неверный выбор."
        exit 1
        ;;
esac

echo ""
echo "--> Все запуски завершены."