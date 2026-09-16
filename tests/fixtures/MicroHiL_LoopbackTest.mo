model MicroHiL_LoopbackTest
  Modelica.Blocks.Interfaces.BooleanInput di_feedback annotation(
    Placement(transformation(origin = {-120, 88}, extent = {{-20, -20}, {20, 20}}), iconTransformation(origin = {-74, 60}, extent = {{-20, -20}, {20, 20}})));
  Modelica.Blocks.Interfaces.RealInput ao_feedback annotation(
    Placement(transformation(origin = {-120, 50}, extent = {{-20, -20}, {20, 20}}), iconTransformation(origin = {-60, 24}, extent = {{-20, -20}, {20, 20}})));
  Modelica.Blocks.Interfaces.RealInput pwm_feedback annotation(
    Placement(transformation(origin = {-120, 12}, extent = {{-20, -20}, {20, 20}}), iconTransformation(origin = {-66, -18}, extent = {{-20, -20}, {20, 20}})));
  Modelica.Blocks.Interfaces.RealOutput ao_command_v annotation(
    Placement(transformation(origin = {110, -50}, extent = {{-10, -10}, {10, 10}}), iconTransformation(origin = {108, -60}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Interfaces.RealOutput pwm_command annotation(
    Placement(transformation(origin = {110, -82}, extent = {{-10, -10}, {10, 10}}), iconTransformation(origin = {108, -80}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Interfaces.BooleanOutput do_command annotation(
    Placement(transformation(origin = {110, -16}, extent = {{-10, -10}, {10, 10}}), iconTransformation(origin = {110, -30}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Interfaces.RealOutput ao_feedback_graph annotation(
    Placement(transformation(origin = {110, 50}, extent = {{-10, -10}, {10, 10}}), iconTransformation(origin = {124, 40}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Interfaces.RealOutput pwm_feedback_graph annotation(
    Placement(transformation(origin = {110, 12}, extent = {{-10, -10}, {10, 10}}), iconTransformation(origin = {124, 20}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Interfaces.BooleanOutput do_feedback_graph annotation(
    Placement(transformation(origin = {110, 88}, extent = {{-10, -10}, {10, 10}}), iconTransformation(origin = {126, 70}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Sources.BooleanPulse booleanPulse(period = 0.25)  annotation(
    Placement(transformation(origin = {-76, -16}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Sources.Sine sine(amplitude = 1, f = 0.25, offset = 1.65)  annotation(
    Placement(transformation(origin = {-76, -50}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Sources.Sine sine1(amplitude = 0.3, f = 0.25, offset = 0.5) annotation(
    Placement(transformation(origin = {-74, -82}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Continuous.FirstOrder firstOrder(T = 0.1, y_start = 0.5)  annotation(
    Placement(transformation(origin = {-14, -82}, extent = {{-10, -10}, {10, 10}})));
  Modelica.Blocks.Nonlinear.Limiter limiter(uMax = 1, uMin = 0)  annotation(
    Placement(transformation(origin = {40, -82}, extent = {{-10, -10}, {10, 10}})));
equation
  connect(di_feedback, do_feedback_graph) annotation(
    Line(points = {{-120, 88}, {110, 88}}, color = {255, 0, 255}));
  connect(ao_feedback, ao_feedback_graph) annotation(
    Line(points = {{-120, 50}, {110, 50}}, color = {0, 0, 127}));
  connect(pwm_feedback, pwm_feedback_graph) annotation(
    Line(points = {{-120, 12}, {110, 12}}, color = {0, 0, 127}));
  connect(booleanPulse.y, do_command) annotation(
    Line(points = {{-64, -16}, {110, -16}}, color = {255, 0, 255}));
  connect(sine.y, ao_command_v) annotation(
    Line(points = {{-65, -50}, {110, -50}}, color = {0, 0, 127}));
  connect(sine1.y, firstOrder.u) annotation(
    Line(points = {{-62, -82}, {-26, -82}}, color = {0, 0, 127}));
  connect(firstOrder.y, limiter.u) annotation(
    Line(points = {{-2, -82}, {28, -82}}, color = {0, 0, 127}));
  connect(limiter.y, pwm_command) annotation(
    Line(points = {{52, -82}, {110, -82}}, color = {0, 0, 127}));

annotation(
    uses(Modelica(version = "4.0.0")),
  experiment(StartTime = 0, StopTime = 200, Tolerance = 1e-06, Interval = 0.02));
end MicroHiL_LoopbackTest;