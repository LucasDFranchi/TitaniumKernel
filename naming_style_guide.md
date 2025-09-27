### Project Naming Conventions (C + C++)

#### **Namespaces**

* Use **lowercase** for namespaces.
  Example: `namespace hw { ... }`

#### **Types (structs, classes, enums)**

* Use **PascalCase** for class, struct, and enum names.
  Example:

  ```cpp
  class TemperatureSensor;
  enum class GpioId { StatusLed, MuxReset };
  ```

#### **Functions**

* Use **snake_case** for free functions (to stay consistent with existing C code).
  Example:

  ```cpp
  GPIOHandler& get_gpio(GpioId id);
  float convert_voltage(float v);
  ```

* Member functions can also use `snake_case` (common in embedded), but **PascalCase** is also acceptable if you want to distinguish C++ classes.
  Just pick one and apply it consistently.

#### **Variables**

* Use **snake_case** for variables, both local and member.
  Example:

  ```cpp
  int sensor_index;
  float ntc_voltage;
  ```

#### **Constants**

* Use **ALL_CAPS with underscores** for macros and compile-time constants in C.
* In C++, prefer `constexpr` or `enum` with **PascalCase**.
  Example:

  ```cpp
  constexpr int MaxSensors = 8;
  ```

#### **Macros**

* Keep them **ALL_CAPS**, only for guards or hardware-specific defines.
  Example:

  ```c
  #define MUX_ADDRESS_0 0x70
  ```

---

### ✅ Examples

```cpp
namespace hw {
    enum class GpioId { StatusLed, MuxReset };

    GPIOHandler& get_gpio(GpioId id);

    class Multiplexer {
       public:
        explicit Multiplexer(TCA9548A* mux_driver);
        kernel_error_st disable();
        const Channel* channels() const;
        size_t channel_count() const;

       private:
        std::array<Channel, TCA9548A::NUM_OF_MUX_CHANNELS> channel_array_;
        TCA9548A* mux_driver_;
    };
}
```

This way:

* **C parts** remain natural (`snake_case` everywhere).
* **C++ types** stand out with `PascalCase`.
* Both styles coexist cleanly, without confusion.
