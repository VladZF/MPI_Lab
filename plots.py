import pandas as pd
import matplotlib.pyplot as plt
import os
import re

RESULTS_DIR = 'Results'

def generate_individual_plots(file_path, output_dir):

    file_name = os.path.basename(file_path)
    print(f"Обработка файла: {file_name}")
    
    try:
        df = pd.read_csv(file_path).sort_values(by='Processes')
        t_serial = df[df['Processes'] == 1]['Time_Seconds'].iloc[0]
    except (IndexError, FileNotFoundError) as e:
        print(f"Ошибка при обработке {file_name}: не найдены данные для 1 процесса или файл. Пропуск. ({e})")
        return

    df['Speedup'] = t_serial / df['Time_Seconds']
    df['Efficiency'] = df['Speedup'] / df['Processes']

    fig, axes = plt.subplots(1, 3, figsize=(20, 5))
    fig.suptitle(f'Анализ производительности для файла: {file_name}', fontsize=16)

    axes[0].plot(df['Processes'], df['Time_Seconds'], marker='o', linestyle='-')
    axes[0].set_title('Время выполнения')
    axes[0].set_xlabel('Количество процессов')
    axes[0].set_ylabel('Время (секунды)')
    axes[0].grid(True)
    axes[0].set_xticks(df['Processes'])

    axes[1].plot(df['Processes'], df['Speedup'], marker='o', linestyle='-', color='g')
    axes[1].set_title('Ускорение')
    axes[1].set_xlabel('Количество процессов')
    axes[1].set_ylabel('Ускорение (S)')
    axes[1].grid(True)
    axes[1].set_xticks(df['Processes'])

    axes[2].plot(df['Processes'], df['Efficiency'], marker='o', linestyle='-', color='r')
    axes[2].set_title('Эффективность')
    axes[2].set_xlabel('Количество процессов')
    axes[2].set_ylabel('Эффективность (E)')
    axes[2].grid(True)
    axes[2].set_xticks(df['Processes'])
    
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    output_filename = os.path.splitext(file_name)[0] + '_plots.png'
    output_path = os.path.join(output_dir, output_filename)
    plt.savefig(output_path)
    print(f"Графики для {file_name} сохранены в {output_path}")


def generate_comparison_plots(file_paths, output_dir):

    print("\nСоздание сравнительных графиков для файлов task2...")

    matrix_dim_subtitle = ""
    if file_paths:
        try:
            df_temp = pd.read_csv(file_paths[0])
            if not df_temp.empty and 'Matrix_Rows' in df_temp.columns and 'Matrix_Cols' in df_temp.columns:
                rows = int(df_temp['Matrix_Rows'].iloc[0])
                cols = int(df_temp['Matrix_Cols'].iloc[0])
                matrix_dim_subtitle = f" (Матрица: {rows}x{cols})"
        except Exception as e:
            print(f"Предупреждение: не удалось прочитать размерность матрицы из {os.path.basename(file_paths[0])}: {e}")

    main_title = f'Сравнительный анализ производительности для task2 (matvec){matrix_dim_subtitle}'
    
    fig, axes = plt.subplots(1, 3, figsize=(22, 6))
    fig.suptitle(main_title, fontsize=16)

    axes[0].set_title('Сравнение времени выполнения')
    axes[0].set_xlabel('Количество процессов')
    axes[0].set_ylabel('Время (секунды)')
    
    axes[1].set_title('Сравнение ускорения')
    axes[1].set_xlabel('Количество процессов')
    axes[1].set_ylabel('Ускорение (S)')
    
    axes[2].set_title('Сравнение эффективности')
    axes[2].set_xlabel('Количество процессов')
    axes[2].set_ylabel('Эффективность (E)')

    for file_path in file_paths:
        file_name = os.path.basename(file_path)
        
        match = re.search(r'matvec_(\w+)\.csv', file_name)
        label = match.group(1) if match else os.path.splitext(file_name)[0]

        try:
            df = pd.read_csv(file_path).sort_values(by='Processes')
            t_serial = df[df['Processes'] == 1]['Time_Seconds'].iloc[0]
        except (IndexError, FileNotFoundError) as e:
            print(f"Ошибка при обработке {file_name}: не найдены данные для 1 процесса или файл. Пропуск. ({e})")
            continue
            
        df['Speedup'] = t_serial / df['Time_Seconds']
        df['Efficiency'] = df['Speedup'] / df['Processes']
        
        axes[0].plot(df['Processes'], df['Time_Seconds'], marker='o', linestyle='-', label=label)
        axes[1].plot(df['Processes'], df['Speedup'], marker='o', linestyle='-', label=label)
        axes[2].plot(df['Processes'], df['Efficiency'], marker='o', linestyle='-', label=label)
        
        axes[0].set_xticks(df['Processes'])
        axes[1].set_xticks(df['Processes'])
        axes[2].set_xticks(df['Processes'])

    for ax in axes:
        ax.legend()
        ax.grid(True)
        
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    output_path = os.path.join(output_dir, 'task2_matvec_comparison_plots.png')
    plt.savefig(output_path)
    print(f"Сравнительные графики сохранены в {output_path}")


def generate_task3_plots(file_path, output_dir):

    file_name = os.path.basename(file_path)
    print(f"\nОбработка файла для Task 3: {file_name}")

    try:
        df = pd.read_csv(file_path)
    except FileNotFoundError:
        print(f"Файл {file_name} не найден. Пропуск.")
        return

    df['Speedup'] = 0.0
    df['Efficiency'] = 0.0

    for size in df['Matrix_Size'].unique():
        mask = df['Matrix_Size'] == size
        df_size = df[mask]
        
        try:
            t_serial = df_size[df_size['Processes'] == 1]['Time_Seconds'].iloc[0]
        except IndexError:
            print(f"Предупреждение: для матрицы {size}x{size} не найдены данные для 1 процесса. Ускорение не будет рассчитано.")
            continue
            
        df.loc[mask, 'Speedup'] = t_serial / df_size['Time_Seconds']
        df.loc[mask, 'Efficiency'] = df.loc[mask, 'Speedup'] / df_size['Processes']

    fig, axes = plt.subplots(1, 3, figsize=(22, 6))
    fig.suptitle('Анализ производительности для Task 3 (алгоритм Кэннона)', fontsize=16)

    axes[0].set_title('Время выполнения от кол-ва процессов')
    axes[1].set_title('Ускорение от кол-ва процессов')
    axes[2].set_title('Эффективность от кол-ва процессов')

    for size in sorted(df['Matrix_Size'].unique()):
        df_plot = df[df['Matrix_Size'] == size].sort_values(by='Processes')
        label = f'Матрица {size}x{size}'
        
        axes[0].plot(df_plot['Processes'], df_plot['Time_Seconds'], marker='o', linestyle='-', label=label)
        axes[1].plot(df_plot['Processes'], df_plot['Speedup'], marker='o', linestyle='-', label=label)
        axes[2].plot(df_plot['Processes'], df_plot['Efficiency'], marker='o', linestyle='-', label=label)

    processes = sorted(df['Processes'].unique())
    for ax in axes:
        ax.set_xlabel('Количество процессов')
        ax.set_xticks(processes)
        ax.grid(True)
        ax.legend()
    
    axes[0].set_ylabel('Время (секунды)')
    axes[1].set_ylabel('Ускорение (S)')
    axes[2].set_ylabel('Эффективность (E)')
    
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    output_path = os.path.join(output_dir, 'task3_cannon_plots.png')
    plt.savefig(output_path)
    print(f"Графики для {file_name} сохранены в {output_path}")


if __name__ == "__main__":
    if not os.path.isdir(RESULTS_DIR):
        print(f"Директория '{RESULTS_DIR}' не найдена.")
        exit()

    all_files = [os.path.join(RESULTS_DIR, f) for f in os.listdir(RESULTS_DIR) if f.endswith('.csv')]
    
    task1_files = [f for f in all_files if 'task1_pi' in f]
    task2_files = [f for f in all_files if 'task2_matvec' in f]
    task3_files = [f for f in all_files if 'task3_cannon' in f]

    if not any([task1_files, task2_files, task3_files]):
        print(f"В директории '{RESULTS_DIR}' не найдено подходящих CSV-файлов.")
        exit()

    
    if task1_files:
        for file_path in task1_files:
            generate_individual_plots(file_path, RESULTS_DIR)
    else:
        print("Файлы для task1 не найдены.")

    if task2_files:
        generate_comparison_plots(task2_files, RESULTS_DIR)
    else:
        print("Файлы для task2 не найдены.")
        
    if task3_files:
        for file_path in task3_files:
            generate_task3_plots(file_path, RESULTS_DIR)
    else:
        print("Файлы для task3 не найдены.")

    print("\nРабота скрипта завершена.")