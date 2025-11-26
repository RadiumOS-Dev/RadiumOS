#include "script.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../Avfs/Avfs.h"
#include "../keyboard/keyboard.h"
#include "../timers/timer.h"

static script_context_t global_context;

// Trim whitespace from beginning and end of string
static void trim(char* str) {
    if (!str || *str == '\0') return;
    
    // Trim leading whitespace
    char* start = str;
    while (*start && (*start == ' ' || *start == '\t')) {
        start++;
    }
    
    // Trim trailing whitespace
    char* end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';
    
    // Move trimmed string to beginning
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// Check if line is a comment
static bool is_comment(const char* line) {
    const char* ptr = line;
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    return (*ptr == '%' || *ptr == '#');
}

// Expand variables in a string (e.g., $VAR or ${VAR})
static void expand_variables(const char* input, char* output, size_t output_size, script_context_t* ctx) {
    const char* in = input;
    char* out = output;
    size_t remaining = output_size - 1;
    
    while (*in && remaining > 0) {
        if (*in == '$') {
            in++;
            char var_name[MAX_VARIABLE_NAME];
            int var_idx = 0;
            
            // Handle ${VAR} syntax
            bool braced = false;
            if (*in == '{') {
                braced = true;
                in++;
            }
            
            // Extract variable name
            while (*in && var_idx < MAX_VARIABLE_NAME - 1) {
                if (braced && *in == '}') {
                    in++;
                    break;
                }
                if (!braced && !(*in >= 'A' && *in <= 'Z') && 
                    !(*in >= 'a' && *in <= 'z') && 
                    !(*in >= '0' && *in <= '9') && 
                    *in != '_') {
                    break;
                }
                var_name[var_idx++] = *in++;
            }
            var_name[var_idx] = '\0';
            
            // Get variable value
            const char* value = script_get_variable(ctx, var_name);
            if (value) {
                size_t value_len = strlen(value);
                if (value_len <= remaining) {
                    strcpy(out, value);
                    out += value_len;
                    remaining -= value_len;
                }
            }
        } else {
            *out++ = *in++;
            remaining--;
        }
    }
    *out = '\0';
}

void script_init(void) {
    memset(&global_context, 0, sizeof(script_context_t));
    global_context.echo_mode = false;
    global_context.last_exit_code = 0;
}

int script_set_variable(script_context_t* ctx, const char* name, const char* value) {
    if (!ctx || !name || !value) return -1;
    
    // Check if variable already exists
    for (int i = 0; i < MAX_SCRIPT_VARIABLES; i++) {
        if (ctx->variables[i].used && strcmp(ctx->variables[i].name, name) == 0) {
            strncpy(ctx->variables[i].value, value, MAX_VARIABLE_VALUE - 1);
            ctx->variables[i].value[MAX_VARIABLE_VALUE - 1] = '\0';
            return 0;
        }
    }
    
    // Find free slot
    for (int i = 0; i < MAX_SCRIPT_VARIABLES; i++) {
        if (!ctx->variables[i].used) {
            strncpy(ctx->variables[i].name, name, MAX_VARIABLE_NAME - 1);
            ctx->variables[i].name[MAX_VARIABLE_NAME - 1] = '\0';
            strncpy(ctx->variables[i].value, value, MAX_VARIABLE_VALUE - 1);
            ctx->variables[i].value[MAX_VARIABLE_VALUE - 1] = '\0';
            ctx->variables[i].used = true;
            return 0;
        }
    }
    
    return -1; // No free slots
}

const char* script_get_variable(script_context_t* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    
    for (int i = 0; i < MAX_SCRIPT_VARIABLES; i++) {
        if (ctx->variables[i].used && strcmp(ctx->variables[i].name, name) == 0) {
            return ctx->variables[i].value;
        }
    }
    
    return NULL;
}

// Execute a single line of script
int script_execute_line(const char* line, script_context_t* ctx) {
    if (!line || !ctx) return -1;
    
    char line_copy[MAX_SCRIPT_LINE_LENGTH];
    strncpy(line_copy, line, MAX_SCRIPT_LINE_LENGTH - 1);
    line_copy[MAX_SCRIPT_LINE_LENGTH - 1] = '\0';
    
    trim(line_copy);
    
    // Skip empty lines and comments
    if (line_copy[0] == '\0' || is_comment(line_copy)) {
        return 0;
    }
    
    // Expand variables
    char expanded[MAX_SCRIPT_LINE_LENGTH];
    expand_variables(line_copy, expanded, MAX_SCRIPT_LINE_LENGTH, ctx);
    
    // Parse command and arguments
    char* argv[MAX_ARGUMENTS];
    int argc = 0;
    
    // Make a copy for tokenization
    char expanded_copy[MAX_SCRIPT_LINE_LENGTH];
    strncpy(expanded_copy, expanded, MAX_SCRIPT_LINE_LENGTH - 1);
    expanded_copy[MAX_SCRIPT_LINE_LENGTH - 1] = '\0';
    
    char* token = strtok(expanded_copy, " \t");
    
    while (token && argc < MAX_ARGUMENTS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    
    if (argc == 0) return 0;
    
    // Built-in script commands
    if (strcmp(argv[0], "echo") == 0) {
        // Print all arguments
        for (int i = 1; i < argc; i++) {
            print(argv[i]);
            if (i < argc - 1) print(" ");
        }
        print("\n");
        return 0;
    }
    else if (strcmp(argv[0], "set") == 0 || strcmp(argv[0], "export") == 0) {
        if (argc < 2) {
            print("Usage: set VAR value\n");
            return -1;
        }
        char value[MAX_VARIABLE_VALUE] = {0};
        for (int i = 2; i < argc; i++) {
            size_t current_len = strlen(value);
            size_t arg_len = strlen(argv[i]);
            size_t space_left = MAX_VARIABLE_VALUE - current_len - 1;
            
            if (arg_len < space_left) {
                strcat(value, argv[i]);
                if (i < argc - 1 && strlen(value) < MAX_VARIABLE_VALUE - 2) {
                    strcat(value, " ");
                }
            }
        }
        script_set_variable(ctx, argv[1], value);
        return 0;
    }
    else if (strcmp(argv[0], "unset") == 0) {
        if (argc < 2) {
            print("Usage: unset VAR\n");
            return -1;
        }
        for (int i = 0; i < MAX_SCRIPT_VARIABLES; i++) {
            if (ctx->variables[i].used && strcmp(ctx->variables[i].name, argv[1]) == 0) {
                ctx->variables[i].used = false;
                return 0;
            }
        }
        return 0;
    }
    else if (strcmp(argv[0], "vars") == 0) {
        print("Variables:\n");
        bool found_any = false;
        for (int i = 0; i < MAX_SCRIPT_VARIABLES; i++) {
            if (ctx->variables[i].used) {
                print("  ");
                print(ctx->variables[i].name);
                print(" = ");
                print(ctx->variables[i].value);
                print("\n");
                found_any = true;
            }
        }
        if (!found_any) {
            print("  (no variables set)\n");
        }
        return 0;
    }
    else if (strcmp(argv[0], "pause") == 0) {
        print("Press any key to continue...");
        keyboard_await();
        print("\n");
        return 0;
    }
    else if (strcmp(argv[0], "sleep") == 0) {
        if (argc < 2) {
            print("Usage: sleep milliseconds\n");
            return -1;
        }
        int ms = atoi(argv[1]);
        if (ms > 0 && ms < 10000) {
            delay(ms);
        }
        return 0;
    }
    else if (strcmp(argv[0], "if_exists") == 0) {
        if (argc < 3) {
            print("Usage: if_exists filename command [args...]\n");
            return -1;
        }
        if (avfs_file_exists(argv[1])) {
            char subcmd[MAX_SCRIPT_LINE_LENGTH] = {0};
            for (int i = 2; i < argc; i++) {
                strcat(subcmd, argv[i]);
                if (i < argc - 1 && strlen(subcmd) < MAX_SCRIPT_LINE_LENGTH - 2) {
                    strcat(subcmd, " ");
                }
            }
            return script_execute_line(subcmd, ctx);
        }
        return 0;
    }
    else {
        // Try to execute as external command
        extern Command commands[];
        extern size_t command_count;
        
        for (size_t i = 0; i < command_count; i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                commands[i].execute(argc, argv);
                return 0;
            }
        }
        
        // Unknown command - silently ignore in scripts
        return 0;
    }
}

// Execute a script file
int script_execute_file(const char* filename) {
    if (!filename) return -1;
    
    // Check if file exists
    if (!avfs_file_exists(filename)) {
        return -1;
    }
    
    int file_size = avfs_get_filesize(filename);
    if (file_size <= 0) {
        return -1;
    }
    
    // Allocate buffer for file content
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        return -1;
    }
    
    // Read file
    if (avfs_read_file(filename, buffer, file_size, 0) != 0) {
        free(buffer);
        return -1;
    }
    buffer[file_size] = '\0';
    
    // Execute line by line
    char line[MAX_SCRIPT_LINE_LENGTH];
    int line_idx = 0;
    int result = 0;
    
    for (int i = 0; i <= file_size; i++) {
        if (buffer[i] == '\n' || buffer[i] == '\0') {
            line[line_idx] = '\0';
            if (line_idx > 0) {
                int exec_result = script_execute_line(line, &global_context);
                if (exec_result != 0 && result == 0) {
                    result = exec_result;
                }
            }
            line_idx = 0;
            if (buffer[i] == '\0') break;
        } else if (line_idx < MAX_SCRIPT_LINE_LENGTH - 1) {
            line[line_idx++] = buffer[i];
        }
    }
    
    free(buffer);
    return result;
}

// Run autoexec script on boot
void script_run_autoexec(void) {
    if (avfs_file_exists("autoexec.rsh")) {
        script_execute_file("autoexec.rsh");
    }
}

