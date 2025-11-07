#!/bin/bash

EXECUTABLE_NAME="task1_pi"

TASKS_DIR="Tasks"
BUILD_DIR="Build"
RESULTS_DIR="Results"

SOURCE_FILE="$TASKS_DIR/$EXECUTABLE_NAME.c"
EXECUTABLE_PATH="$BUILD_DIR/$EXECUTABLE_NAME"
TOTAL_POINTS=100000000
RESULTS_FILE="$RESULTS_DIR/$EXECUTABLE_NAME.csv"


echo "--> Проверка директории для сборки..."
mkdir -p $BUILD_DIR
echo "--> Директория '$BUILD_DIR' готова."
echo ""

echo "--> Проверка директории для результатов..."
mkdir -p $RESULTS_DIR
echo "--> Директория '$RESULTS_DIR' готова."
echo ""

echo "--> Компиляция программы: $SOURCE_FILE"
mpicc $SOURCE_FILE -o $EXECUTABLE_PATH

if [ $? -ne 0 ]; then
    echo "--> ОШИБКА КОМПИЛЯЦИИ. Пожалуйста, проверьте исходный код."
    exit 1
else
    echo "--> Компиляция прошла успешно. Исполняемый файл: $EXECUTABLE_PATH"
fi
echo ""

echo "--> Создание файла для результатов: $RESULTS_FILE"
echo "Processes,Pi_Value,Time_Seconds" > $RESULTS_FILE
echo ""

PROCESSES_TO_RUN=(1 2 4 8)

echo "--> Запуск вычислений с $TOTAL_POINTS точек..."
echo "========================================================"

for N in "${PROCESSES_TO_RUN[@]}"
do
    echo -n "--> ВЫПОЛНЕНИЕ НА $N ПРОЦЕССАХ... "
    mpiexec -np $N $EXECUTABLE_PATH $TOTAL_POINTS >> $RESULTS_FILE
    echo "[ЗАВЕРШЕНО]"
    echo "--------------------------------------------------------"
done

echo "--> Все запуски завершены. Результаты записаны в $RESULTS_FILE"