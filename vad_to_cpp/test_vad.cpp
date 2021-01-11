#include <torch/torch.h>
#include <torch/script.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <unistd.h>


using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using namespace std;

int main(int args, char * argv[]){

  //载入jit格式的模型
  torch::jit::script::Module module = torch::jit::load("/data/test_pytorch_vad/vad/model.jit");

  //c++ vector example
  vector<float> scales(1024, 1);  

  //c++ vector格式转到torch tensor
  torch::Tensor input1 = torch::tensor(scales);
  input1 = torch::reshape(input1, {1,1,-1});
  std::vector<torch::jit::IValue> inputs;
  inputs.push_back(torch::rand({1, 1, 1024}));
  
  //演示连续做1000次c++预测
  torch::Tensor model_output;
  for (int i=0; i<100; i++){
    //usleep(500000);
    high_resolution_clock::time_point beginTime = high_resolution_clock::now();
    model_output = module.forward(inputs).toTensor();
    high_resolution_clock::time_point endTime = high_resolution_clock::now();
    milliseconds timeInterval = std::chrono::duration_cast<milliseconds>(endTime - beginTime);
    std::cout << i << " "<< timeInterval.count() << "ms";  
	model_output = torch::reshape(model_output, {-1});  
	
	if (model_output[0].item().toFloat() > model_output[1].item().toFloat()){
		cout<<" True\n";}
	else cout<<" False\n";
  }

}
