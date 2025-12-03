#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ==================== СТРУКТУРЫ ДАННЫХ ====================

typedef struct Node {
    unsigned char symbol;
    unsigned int freq;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct MinHeap {
    int size;
    int capacity;
    Node **array;
} MinHeap;

typedef struct HuffmanCode {
    unsigned char symbol;
    char code[256];
    int length;
} HuffmanCode;

// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================

void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// ==================== МИНИМАЛЬНАЯ КУЧА ====================

static void swap_nodes(Node** a, Node** b) {
    Node* temp = *a;
    *a = *b;
    *b = temp;
}

static void minheapify(MinHeap* heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && 
        heap->array[left]->freq < heap->array[smallest]->freq)
        smallest = left;

    if (right < heap->size && 
        heap->array[right]->freq < heap->array[smallest]->freq)
        smallest = right;

    if (smallest != idx) {
        swap_nodes(&heap->array[smallest], &heap->array[idx]);
        minheapify(heap, smallest);
    }
}

Node* create_node(unsigned char symbol, unsigned int freq) {
    Node* node = (Node*)safe_malloc(sizeof(Node));
    node->symbol = symbol;
    node->freq = freq;
    node->left = NULL;
    node->right = NULL;
    return node;
}

MinHeap* create_minheap(int capacity) {
    MinHeap* heap = (MinHeap*)safe_malloc(sizeof(MinHeap));
    heap->size = 0;
    heap->capacity = capacity;
    heap->array = (Node**)safe_malloc(capacity * sizeof(Node*));
    return heap;
}

Node* extract_min(MinHeap* heap) {
    if (heap->size == 0)
        return NULL;

    Node* min = heap->array[0];
    heap->array[0] = heap->array[heap->size - 1];
    heap->size--;
    minheapify(heap, 0);
    return min;
}

void insert_minheap(MinHeap* heap, Node* node) {
    if (heap->size >= heap->capacity) {
        heap->capacity *= 2;
        heap->array = (Node**)realloc(heap->array, heap->capacity * sizeof(Node*));
        if (!heap->array) {
            fprintf(stderr, "Ошибка перевыделения памяти для кучи\n");
            exit(EXIT_FAILURE);
        }
    }
    
    int i = heap->size;
    heap->array[i] = node;
    heap->size++;
    
    // Просеивание вверх
    while (i > 0 && heap->array[(i - 1) / 2]->freq > heap->array[i]->freq) {
        swap_nodes(&heap->array[i], &heap->array[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

void build_minheap(MinHeap* heap) {
    int n = heap->size - 1;
    for (int i = (n - 1) / 2; i >= 0; i--)
        minheapify(heap, i);
}

void free_minheap(MinHeap* heap) {
    if (heap) {
        free(heap->array);
        free(heap);
    }
}

// ==================== ОСНОВНЫЕ ФУНКЦИИ ХАФФМАНА ====================

unsigned int* count_frequencies(const char* filename, int* unique_count, 
                                 size_t* total_bytes) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла: %s\n", filename);
        return NULL;
    }
    
    unsigned int* freq = (unsigned int*)calloc(256, sizeof(unsigned int));
    if (!freq) {
        fclose(file);
        return NULL;
    }
    
    unsigned char buffer[4096];
    size_t bytes_read;
    *total_bytes = 0;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        *total_bytes += bytes_read;
        for (size_t i = 0; i < bytes_read; i++)
            freq[buffer[i]]++;
    }
    
    fclose(file);
    
    *unique_count = 0;
    for (int i = 0; i < 256; i++)
        if (freq[i] > 0) (*unique_count)++;
    
    return freq;
}

Node* build_huffman_tree(unsigned int* freq) {
    MinHeap* heap = create_minheap(256);
    
    for (int i = 0; i < 256; i++)
        if (freq[i] > 0)
            heap->array[heap->size++] = create_node(i, freq[i]);
    
    if (heap->size == 0) {
        free_minheap(heap);
        return NULL;
    }
    
    if (heap->size == 1) {
        Node* single_node = heap->array[0];
        Node* root = create_node(0, single_node->freq);
        root->left = single_node;
        root->right = create_node(single_node->symbol, 0);
        free_minheap(heap);
        return root;
    }
    
    build_minheap(heap);
    
    while (heap->size > 1) {
        Node* left = extract_min(heap);
        Node* right = extract_min(heap);
        
        Node* parent = create_node(0, left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        
        insert_minheap(heap, parent);
    }
    
    Node* root = extract_min(heap);
    free_minheap(heap);
    return root;
}

void generate_codes_recursive(Node* root, char* current_code, int depth, 
                              HuffmanCode* codes, int* index) {
    if (!root) return;
    
    if (!root->left && !root->right) {
        codes[*index].symbol = root->symbol;
        codes[*index].length = depth;
        if (depth > 0) {
            strncpy(codes[*index].code, current_code, depth);
        }
        codes[*index].code[depth] = '\0';
        (*index)++;
        return;
    }
    
    if (root->left) {
        current_code[depth] = '0';
        generate_codes_recursive(root->left, current_code, depth + 1, codes, index);
    }
    
    if (root->right) {
        current_code[depth] = '1';
        generate_codes_recursive(root->right, current_code, depth + 1, codes, index);
    }
}

HuffmanCode* get_huffman_codes(Node* root, int unique_count) {
    if (!root || unique_count == 0) return NULL;
    
    HuffmanCode* codes = (HuffmanCode*)safe_malloc(unique_count * sizeof(HuffmanCode));
    char current_code[256] = {0};
    int index = 0;
    
    generate_codes_recursive(root, current_code, 0, codes, &index);
    return codes;
}

const char* find_code(HuffmanCode* codes, int unique_count, unsigned char symbol) {
    for (int i = 0; i < unique_count; i++)
        if (codes[i].symbol == symbol)
            return codes[i].code;
    return NULL;
}

void free_tree(Node* root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

// ==================== РЕЖИМ КОДИРОВАНИЯ ====================

void encode_file(const char* input_filename, const char* output_filename) {
    printf("=== РЕЖИМ КОДИРОВАНИЯ ===\n\n");
    
    unsigned int* freq = NULL;
    Node* root = NULL;
    HuffmanCode* codes = NULL;
    FILE* input_file = NULL;
    FILE* output_file = NULL;
    
    printf("1. Анализ файла и подсчет частот символов...\n");
    int unique_count = 0;
    size_t original_size = 0;
    
    freq = count_frequencies(input_filename, &unique_count, &original_size);
    if (!freq) {
        printf("Ошибка: не удалось прочитать файл\n");
        goto cleanup;
    }
    
    if (original_size == 0) {
        printf("Ошибка: файл пуст\n");
        goto cleanup;
    }
    
    printf("   Уникальных символов: %d\n", unique_count);
    printf("   Общий размер: %lu байт\n", original_size);
    
    printf("\n2. Построение дерева Хаффмана...\n");
    root = build_huffman_tree(freq);
    if (!root) {
        printf("Ошибка построения дерева\n");
        goto cleanup;
    }
    
    printf("\n3. Генерация кодов Хаффмана...\n");
    codes = get_huffman_codes(root, unique_count);
    if (!codes) {
        printf("Ошибка генерации кодов\n");
        goto cleanup;
    }
    
    printf("\n4. Кодирование и сохранение...\n");
    
    input_file = fopen(input_filename, "rb");
    if (!input_file) {
        printf("Ошибка открытия входного файла\n");
        goto cleanup;
    }
    
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        printf("Ошибка открытия выходного файла\n");
        goto cleanup;
    }
    
    // Запись заголовка
    fwrite(&unique_count, sizeof(int), 1, output_file);
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            unsigned char symbol = i;
            fwrite(&symbol, sizeof(unsigned char), 1, output_file);
            fwrite(&freq[i], sizeof(unsigned int), 1, output_file);
        }
    }
    
    // Кодирование данных
    unsigned char read_buffer;
    unsigned char write_buffer = 0;
    int bit_position = 0;
    size_t encoded_bits = 0;
    size_t encoded_bytes = 0;
    
    while (fread(&read_buffer, 1, 1, input_file) == 1) {
        const char* code = find_code(codes, unique_count, read_buffer);
        if (code) {
            for (int i = 0; code[i] != '\0'; i++) {
                write_buffer <<= 1;
                if (code[i] == '1') write_buffer |= 1;
                bit_position++;
                encoded_bits++;
                
                if (bit_position == 8) {
                    fwrite(&write_buffer, 1, 1, output_file);
                    encoded_bytes++;
                    write_buffer = 0;
                    bit_position = 0;
                }
            }
        }
    }
    
    // Запись остатка
    if (bit_position > 0) {
        write_buffer <<= (8 - bit_position);
        fwrite(&write_buffer, 1, 1, output_file);
        encoded_bytes++;
    }
    
    // Сохранение информации о последнем байте
    fwrite(&bit_position, sizeof(int), 1, output_file);
    
    // Статистика
    printf("\n5. Статистика сжатия:\n");
    size_t header_size = sizeof(int) + unique_count * (sizeof(unsigned char) + sizeof(unsigned int));
    size_t total_compressed_size = header_size + encoded_bytes + sizeof(int);
    
    printf("   Исходный размер:    %lu байт\n", original_size);
    printf("   Размер заголовка:   %lu байт\n", header_size);
    printf("   Закодированные данные: %lu байт\n", encoded_bytes);
    printf("   Общий размер:       %lu байт\n", total_compressed_size);
    
    if (original_size > 0) {
        double compression_ratio = (1.0 - (double)total_compressed_size / original_size) * 100;
        printf("   Коэффициент сжатия: %.2f%%\n", compression_ratio);
    }
    
    printf("\nКодирование завершено успешно!\n");
    printf("Результат сохранен в: %s\n", output_filename);

cleanup:
    if (input_file) fclose(input_file);
    if (output_file) fclose(output_file);
    if (freq) free(freq);
    if (root) free_tree(root);
    if (codes) free(codes);
}

// ==================== РЕЖИМ ДЕКОДИРОВАНИЯ ====================

void decode_file(const char* input_filename, const char* output_filename) {
    printf("=== РЕЖИМ ДЕКОДИРОВАНИЯ ===\n\n");
    
    FILE* input_file = NULL;
    FILE* output_file = NULL;
    unsigned int* freq = NULL;
    Node* root = NULL;
    
    printf("1. Чтение заголовка сжатого файла...\n");
    
    input_file = fopen(input_filename, "rb");
    if (!input_file) {
        printf("Ошибка открытия входного файла\n");
        goto decode_cleanup;
    }
    
    int unique_count = 0;
    if (fread(&unique_count, sizeof(int), 1, input_file) != 1) {
        printf("Ошибка: неверный формат файла\n");
        goto decode_cleanup;
    }
    
    if (unique_count <= 0 || unique_count > 256) {
        printf("Ошибка: некорректное количество символов: %d\n", unique_count);
        goto decode_cleanup;
    }
    
    printf("   Уникальных символов в таблице: %d\n", unique_count);
    
    freq = (unsigned int*)calloc(256, sizeof(unsigned int));
    if (!freq) {
        printf("Ошибка выделения памяти\n");
        goto decode_cleanup;
    }
    
    for (int i = 0; i < unique_count; i++) {
        unsigned char symbol;
        unsigned int frequency;
        
        if (fread(&symbol, sizeof(unsigned char), 1, input_file) != 1 ||
            fread(&frequency, sizeof(unsigned int), 1, input_file) != 1) {
            printf("Ошибка чтения таблицы частот\n");
            goto decode_cleanup;
        }
        freq[symbol] = frequency;
    }
    
    printf("\n2. Восстановление дерева Хаффмана...\n");
    root = build_huffman_tree(freq);
    if (!root) {
        printf("Ошибка восстановления дерева\n");
        goto decode_cleanup;
    }
    
    printf("\n3. Декодирование данных...\n");
    
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        printf("Ошибка открытия выходного файла\n");
        goto decode_cleanup;
    }
    
    // Определение размера данных
    fseek(input_file, 0, SEEK_END);
    long file_end = ftell(input_file);
    
    // Чтение информации о последнем байте
    fseek(input_file, -sizeof(int), SEEK_END);
    int last_byte_bits = 0;
    if (fread(&last_byte_bits, sizeof(int), 1, input_file) != 1) {
        printf("Ошибка чтения информации о последнем байте\n");
        goto decode_cleanup;
    }
    
    if (last_byte_bits < 0 || last_byte_bits > 8) {
        printf("Ошибка: некорректное количество бит в последнем байте: %d\n", last_byte_bits);
        goto decode_cleanup;
    }
    
    // Вычисление позиции начала данных
    long header_size = sizeof(int) + unique_count * (sizeof(unsigned char) + sizeof(unsigned int));
    long data_start = header_size;
    long data_size = file_end - data_start - sizeof(int);
    
    if (data_size < 0) {
        printf("Ошибка: некорректный размер данных\n");
        goto decode_cleanup;
    }
    
    fseek(input_file, data_start, SEEK_SET);
    
    // Декодирование
    Node* current = root;
    unsigned char byte;
    int bits_processed = 0;
    int total_bits = 0;
    
    if (data_size > 0) {
        if (last_byte_bits == 0) {
            total_bits = data_size * 8;
        } else {
            total_bits = (data_size - 1) * 8 + last_byte_bits;
        }
    }
    
    size_t decoded_bytes = 0;
    
    for (long i = 0; i < data_size; i++) {
        if (fread(&byte, 1, 1, input_file) != 1) break;
        
        int bits_to_process = (i == data_size - 1 && last_byte_bits > 0) ? last_byte_bits : 8;
        
        for (int bit = 7; bit >= 8 - bits_to_process; bit--) {
            int current_bit = (byte >> bit) & 1;
            
            if (current_bit == 0) {
                current = current->left;
            } else {
                current = current->right;
            }
            
            if (!current) {
                printf("Ошибка: достигнут NULL узел при декодировании\n");
                goto decode_cleanup;
            }
            
            bits_processed++;
            
            if (!current->left && !current->right) {
                fwrite(&current->symbol, 1, 1, output_file);
                decoded_bytes++;
                current = root;
            }
            
            if (bits_processed >= total_bits) break;
        }
        if (bits_processed >= total_bits) break;
    }
    
    printf("\n4. Результаты декодирования:\n");
    printf("   Декодировано байт: %lu\n", decoded_bytes);
    printf("   Обработано бит: %d\n", bits_processed);
    printf("\nДекодирование завершено успешно!\n");
    printf("Результат сохранен в: %s\n", output_filename);

decode_cleanup:
    if (input_file) fclose(input_file);
    if (output_file) fclose(output_file);
    if (freq) free(freq);
    if (root) free_tree(root);
}

// ==================== ГЛАВНАЯ ФУНКЦИЯ ====================

void print_help() {
    printf("\nПрограмма алгоритма Хаффмана для сжатия и восстановления файлов\n");
    printf("===============================================================\n\n");
    printf("Использование:\n");
    printf("  ./huffman <режим> <входной_файл> <выходной_файл>\n\n");
    printf("Режимы работы:\n");
    printf("  encode  - сжатие файла (кодирование)\n");
    printf("  decode  - восстановление файла (декодирование)\n\n");
    printf("Примеры:\n");
    printf("  ./huffman encode document.txt compressed.bin\n");
    printf("  ./huffman decode compressed.bin restored.txt\n\n");
    printf("Проверка корректности:\n");
    printf("  diff document.txt restored.txt\n");
}

int main(int argc, char* argv[]) {
    printf("Алгоритм Хаффмана - сжатие без потерь\n");
    printf("=====================================\n");
    
    if (argc != 4) {
        print_help();
        return 1;
    }
    
    char* mode = argv[1];
    char* input_file = argv[2];
    char* output_file = argv[3];
    
    // Проверка существования входного файла
    FILE* test = fopen(input_file, "rb");
    if (!test) {
        printf("Ошибка: входной файл '%s' не найден\n", input_file);
        return 1;
    }
    fclose(test);
    
    if (strcmp(mode, "encode") == 0) {
        encode_file(input_file, output_file);
    } 
    else if (strcmp(mode, "decode") == 0) {
        decode_file(input_file, output_file);
    }
    else {
        printf("Ошибка: неверный режим '%s'\n", mode);
        printf("Используйте 'encode' или 'decode'\n");
        return 1;
    }
    
    return 0;
}