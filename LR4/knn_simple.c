#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

// Параметры
#define ELEMENTS_EDU    100000    // Размер обучающей выборки
#define ELEMENTS_TEST   1000      // Размер тестовой выборки
#define FIELDS          5         // Количество параметров квартиры
#define MAX_FLOORS      50        // Максимальный этаж
#define MAX_ROOMS       5         // Максимальное количество комнат
#define MAX_SQUARE      300       // Максимальная площадь
#define DISTINCTS       3         // Количество районов
#define KNN             35        // Количество ближайших соседей

// Глобальные данные (только для чтения)
int apartments[ELEMENTS_EDU][FIELDS];
int test_apartments[ELEMENTS_TEST][FIELDS];

const double base_price_per_sqm = 100.0;
const double floorK = 0.2;
const double roomK = 0.8;
const double distinctK[3] = {0.6, 1.0, 1.8};

const int k1 = MAX_SQUARE / MAX_FLOORS;
const int k2 = MAX_SQUARE / MAX_ROOMS;
const int k3 = 1;
const int k4 = MAX_SQUARE / DISTINCTS;

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    int num_threads;
    int start_idx;
    int end_idx;
} ThreadData;

// Структура для сравнения по расстоянию
typedef struct {
    int dist;
    int id;
} DistancePair;

// Функция сравнения для qsort
int compare(const void *a, const void *b) {
    return ((DistancePair *)a)->dist - ((DistancePair *)b)->dist;
}

// Функция потока
void *process_test_batch(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    // Выделяем массивы в куче
    DistancePair *euclid = malloc(ELEMENTS_EDU * sizeof(DistancePair));
    DistancePair *manhattan = malloc(ELEMENTS_EDU * sizeof(DistancePair));

    if (!euclid || !manhattan) {
        fprintf(stderr, "Error: failed to allocate memory in thread %d\n", data->thread_id);
        pthread_exit(NULL);
    }

    for (int i = data->start_idx; i < data->end_idx; i++) {
        // Заполняем расстояния
        for (int j = 0; j < ELEMENTS_EDU; j++) {
            double d_floor = test_apartments[i][0] - apartments[j][0];
            double d_room  = test_apartments[i][1] - apartments[j][1];
            double d_sq    = test_apartments[i][2] - apartments[j][2];
            double d_dist  = test_apartments[i][3] - apartments[j][3];

            euclid[j].dist = (int)sqrt(
                k1 * d_floor * d_floor +
                k2 * d_room  * d_room  +
                k3 * d_sq    * d_sq    +
                k4 * d_dist  * d_dist
            );
            euclid[j].id = j;

            manhattan[j].dist = (int)(
                k1 * fabs(d_floor) +
                k2 * fabs(d_room)  +
                k3 * fabs(d_sq)    +
                k4 * fabs(d_dist)
            );
            manhattan[j].id = j;
        }

        // Сортируем
        qsort(euclid, ELEMENTS_EDU, sizeof(DistancePair), compare);
        qsort(manhattan, ELEMENTS_EDU, sizeof(DistancePair), compare);

        // Предсказание
        int eucl_price = 0, manh_price = 0;
        for (int k = 0; k < KNN; k++) {
            eucl_price += apartments[euclid[k].id][4];
            manh_price += apartments[manhattan[k].id][4];
        }
        eucl_price /= KNN;
        manh_price /= KNN;

        double eucl_acc = (double)eucl_price / test_apartments[i][4];
        double manh_acc = (double)manh_price / test_apartments[i][4];
    }

    // Освобождение памяти
    free(euclid);
    free(manhattan);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Объявление переменных
    int num_threads = 4;
    int batch_size;

    struct timeval start, end;

    pthread_t *threads;
    ThreadData *thread_data;

    int i, t;

    // Чтение количества потоков
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads < 1 || num_threads > 64) {
            printf("Количество потоков должно быть от 1 до 64.\n");
            return 1;
        }
    }

    srand((unsigned int)time(NULL));

    // Генерация обучающих данных
    for (i = 0; i < ELEMENTS_EDU; i++) {
        apartments[i][0] = (int)((rand() / (double)RAND_MAX) * MAX_FLOORS) + 1;
        apartments[i][1] = (int)((rand() / (double)RAND_MAX) * MAX_ROOMS) + 1;
        int min_sq = apartments[i][1] * 10;
        apartments[i][2] = (int)((rand() / (double)RAND_MAX) * (MAX_SQUARE - min_sq + 1)) + min_sq;
        apartments[i][3] = (int)((rand() / (double)RAND_MAX) * DISTINCTS);
        apartments[i][4] = (int)(
            (apartments[i][0] * floorK) *
            (apartments[i][1] * roomK) *
            (apartments[i][2] * base_price_per_sqm) *
            distinctK[apartments[i][3]]
        );
    }

    // Генерация тестовых данных
    for (i = 0; i < ELEMENTS_TEST; i++) {
        test_apartments[i][0] = (int)((rand() / (double)RAND_MAX) * MAX_FLOORS) + 1;
        test_apartments[i][1] = (int)((rand() / (double)RAND_MAX) * MAX_ROOMS) + 1;
        int min_sq = test_apartments[i][1] * 10;
        test_apartments[i][2] = (int)((rand() / (double)RAND_MAX) * (MAX_SQUARE - min_sq + 1)) + min_sq;
        test_apartments[i][3] = (int)((rand() / (double)RAND_MAX) * DISTINCTS);
        test_apartments[i][4] = (int)(
            (test_apartments[i][0] * floorK) *
            (test_apartments[i][1] * roomK) *
            (test_apartments[i][2] * base_price_per_sqm) *
            distinctK[test_apartments[i][3]]
        );
    }

    // Выделение памяти для массивов потоков
    threads = malloc(num_threads * sizeof(pthread_t));
    thread_data = malloc(num_threads * sizeof(ThreadData));

    if (!threads || !thread_data) {
        fprintf(stderr, "Ошибка выделения памяти для потоков\n");
        return 1;
    }

    batch_size = ELEMENTS_TEST / num_threads;

    // Замер времени
    gettimeofday(&start, NULL);

    // Создание потоков
    for (t = 0; t < num_threads; t++) {
        thread_data[t].thread_id = t;
        thread_data[t].start_idx = t * batch_size;
        thread_data[t].end_idx = (t == num_threads - 1) ? ELEMENTS_TEST : (t + 1) * batch_size;

        int ret = pthread_create(&threads[t], NULL, process_test_batch, (void *)&thread_data[t]);
        if (ret != 0) {
            fprintf(stderr, "Ошибка создания потока %d\n", t);
            free(threads);
            free(thread_data);
            return 1;
        }
    }

    // Ожидание завершения потоков
    for (t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    // Окончание замера времени
    gettimeofday(&end, NULL);
    double total_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("%.6f\n", total_time);

    // Освобождение памяти
    free(threads);
    free(thread_data);

    return 0;
}
