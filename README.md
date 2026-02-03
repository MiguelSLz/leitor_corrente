# leitor_corrente
Pacote de ROS2 responsável por publicar dados de corrente elétrica [A]. É feito também a comunicaçaõ serial com a esp32 e o cálculo dos dados brutos do ADC para se obter as correntes.

## Dependências

* ROS 2 Humble (ou compatível)
* `rclcpp`
* `std_msgs`

```bash
cd ~/ros2_ws
colcon build --packages-select leitor_corrente
source install/setup.bash
```

```bash
ros2 run leitor_corrente leitor_corrente
```