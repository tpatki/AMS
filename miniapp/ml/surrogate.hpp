#ifndef _SURROGATE_EOS_HPP_
#define _SURROGATE_EOS_HPP_

#include <string>

#ifdef __ENABLE_TORCH__
#include <torch/script.h>  // One-stop header.
#endif

using namespace std;

#include "app/eos.hpp"
#include "app/eos_idealgas.hpp"
#include "utils/utils_data.hpp"

#ifdef __ENABLE_TORCH__

//! An implementation for a surrogate model
template <typename TypeInValue>
class SurrogateModel {

    static_assert(
        std::is_floating_point<TypeInValue>::value,
        "HDCache supports floating-point values (floats, doubles, or long doubles) only!");

    using data_handler = ams::DataHandler<TypeInValue>;  // utils to handle float data

    string model_path;
    bool is_cpu;
    torch::jit::script::Module module;
    c10::TensorOptions tensorOptions;

   private:
    inline at::Tensor arrayToTensor(long numRows, long numCols, TypeInValue** array) {
        c10::SmallVector<at::Tensor, 8> Tensors;
        for (int i = 0; i < numCols; i++) {
            Tensors.push_back(
                torch::from_blob((TypeInValue*)array[i], {numRows, 1}, tensorOptions));
        }
        at::Tensor tensor = at::reshape(at::cat(Tensors, 1), {numRows, numCols});
        return tensor;
    }

    inline void tensorToArray(at::Tensor tensor, long numRows, long numCols,
                              TypeInValue** array) {
        // Transpose to get continuous memory and
        // perform single memcpy.
        tensor = tensor.transpose(1, 0);
        if (is_cpu) {
            std::cout << "Copying Host Outputs\n";
            for (long j = 0; j < numCols; j++) {
                auto tmp = tensor[j].contiguous();
                TypeInValue* ptr = tmp.data_ptr<TypeInValue>();
                HtoHMemcpy(array[j], ptr, sizeof(TypeInValue) * numRows);
            }
        } else {
            std::cout << "Copying Device Outputs\n";
            for (long j = 0; j < numCols; j++) {
                auto tmp = tensor[j].contiguous();
                TypeInValue* ptr = tmp.data_ptr<TypeInValue>();
                DtoDMemcpy(array[j], ptr, sizeof(TypeInValue) * numRows);
            }
        }
    }

    void loadModel(at::ScalarType dType, c10::Device&& device) {
        try {
            std::cout << "File Name :" << model_path << "\n";
            module = torch::jit::load(model_path);
            module.to(device);
            module.to(dType);
            tensorOptions = torch::TensorOptions().dtype(dType).device(device);
        } catch (const c10::Error& e) {
            std::cerr << "error loading the model\n";
            exit(-1);
        }
    }

    inline void _evaluate(long num_elements, long num_in, size_t num_out, TypeInValue** inputs,
                          TypeInValue** outputs) {
        auto input = arrayToTensor(num_elements, num_in, inputs);
        std::cout << "Shape I:" << input.sizes() << "\n";
        at::Tensor output = module.forward({input}).toTensor();
        std::cout << "Shape O:" << output.sizes() << "\n";
        tensorToArray(output, num_elements, num_out, outputs);
    }

   public:
    template <typename T = TypeInValue,
              std::enable_if_t<std::is_same<T, double>::value>* = nullptr>
    SurrogateModel(const char* model_path, bool is_cpu = true)
        : model_path(model_path), is_cpu(is_cpu) {
        std::cout << "Using double precision models\n";
        if (is_cpu)
            loadModel(torch::kFloat64, torch::Device("cpu"));
        else
            loadModel(torch::kFloat64, torch::Device("cuda"));
    }

    template <typename T = TypeInValue,
              std::enable_if_t<std::is_same<T, float>::value>* = nullptr>
    SurrogateModel(const char* model_path, bool is_cpu = true)
        : model_path(model_path), is_cpu(is_cpu) {
        std::cout << "Using single precision models\n";
        if (is_cpu)
            loadModel(torch::kFloat32, torch::Device("cpu"));
        else
            loadModel(torch::kFloat32, torch::Device("cuda"));
    }

    template <typename T, std::enable_if_t<std::is_same<TypeInValue, T>::value>* = nullptr>
    void Eval(long num_elements, long num_in, size_t num_out, T** inputs, T** outputs) {
        _evaluate(num_elements, num_in, num_out, inputs, outputs);
    }

    template <typename T, std::enable_if_t<!std::is_same<TypeInValue, T>::value>* = nullptr>
    void Eval(long num_elements, long num_in, size_t num_out, T** inputs, T** outputs) {

        std::vector<TypeInValue*> cinputs;
        std::vector<TypeInValue*> coutputs;
        for (int i = 0; i < num_in; i++) {
            cinputs.emplace_back(data_handler::cast_to_typevalue(num_elements, inputs[i]));
        }

        for (int i = 0; i < num_out; i++) {
            coutputs.emplace_back(new TypeInValue[num_elements]);
        }

        _evaluate(num_elements, num_in, num_out, static_cast<TypeInValue**>(cinputs.data()),
                  static_cast<TypeInValue**>(coutputs.data()));

        for (int i = 0; i < num_out; i++) {
            delete[] cinputs[i];
        }

        for (int i = 0; i < num_out; i++) {
            ams::DataHandler<T>::cast_from_typevalue(num_elements, outputs[i], coutputs[i]);
            delete[] coutputs[i];
        }
    }

    template <typename T>
    void Eval(long num_elements, std::vector<T*> inputs, std::vector<T*> outputs) {
        Eval(num_elements, inputs.size(), outputs.size(), static_cast<T**>(inputs.data()),
             static_cast<T**>(outputs.data()));
    }
};

#else

//! An implementation for a surrogate model
template <typename TypeInValue>
class SurrogateModel {

    static_assert(
        std::is_floating_point<TypeInValue>::value,
        "HDCache supports floating-point values (floats, doubles, or long doubles) only!");

    using data_handler = ams::DataHandler<TypeInValue>;  // utils to handle float data

    string model_path;
    bool is_cpu;

private:

public:
    template <typename T = TypeInValue,
              std::enable_if_t<std::is_same<T, double>::value>* = nullptr>
    SurrogateModel(const char* model_path, bool is_cpu = true)
        : model_path(model_path), is_cpu(is_cpu) {
    }

    template <typename T = TypeInValue,
              std::enable_if_t<std::is_same<T, float>::value>* = nullptr>
    SurrogateModel(const char* model_path, bool is_cpu = true)
        : model_path(model_path), is_cpu(is_cpu) {
    }

    template <typename T, std::enable_if_t<std::is_same<TypeInValue, T>::value>* = nullptr>
    void Eval(long num_elements, long num_in, size_t num_out, T** inputs, T** outputs) {
    }

    template <typename T, std::enable_if_t<!std::is_same<TypeInValue, T>::value>* = nullptr>
    void Eval(long num_elements, long num_in, size_t num_out, T** inputs, T** outputs) {
    }

    template <typename T>
    void Eval(long num_elements, std::vector<T*> inputs, std::vector<T*> outputs) {
    }
};
#endif
#endif
