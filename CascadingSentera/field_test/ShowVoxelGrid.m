% ShowVoxelGrid
% Reads voxel grid as .csv, displays to show it works

% controls
VoxGridCSVPath = "VoxGrid3.csv";

% read in csv
Matrix = csvread(VoxGridCSVPath,1,0);
value = Matrix(:,1);
x = Matrix(:,2);
y = Matrix(:,3);
z = Matrix(:,4);

% plot results
figure(1);
subplot(1,2,1); scatter(x, y, 15, z, 'filled');
subplot(1,2,2); scatter3(x,y,z,15,z, 'filled');
colormap autumn;
