#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class TaskTemplate {
public:
    virtual ~TaskTemplate() = default;

    // Wrapper passed to FreeRTOS
    static void loop(void* args) {
        auto* self = static_cast<TaskTemplate*>(args);
        if (self) {
            self->run();
        }
        vTaskDelete(nullptr);  // cleanup when run() returns
    }

    // Derived classes must implement this
    virtual void run() = 0;

    // Helper to create the task
    void start(const char* name, uint16_t stackDepth, UBaseType_t priority) {
        xTaskCreate(&TaskTemplate::loop, name, stackDepth, this, priority, nullptr);
    }
};
