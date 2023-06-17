% curve fitting toolbox needed
table = table2array(readtable('Temp_vs_ADC_graph.csv'));
x = table(:,1);
y = table(:,2);

f = fit(x,y,'poly4')

plot(f, x, y, 'o')