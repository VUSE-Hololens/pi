% ShowVoxelGrid
% Reads voxel grid as .csv, displays to show it works

% Note: Hololens Coordinate system:
    % x, z: horizontal plane (x left-right, z forward-back)
    % y: vertical direction (up-down)

% controls
VoxGridCSVPath = "mound_bottom/VoxGrid2.csv";

% read in csv
Matrix = csvread(VoxGridCSVPath,1,0);
orig_value = Matrix(:,1);
x = Matrix(:,2);
y = Matrix(:,3);
z = Matrix(:,4);

% adjust ndvi readings
value = TrimScale(orig_value, 0, 255, 0, 255, -1, 1);

% plot results
figure(1);
subplot(2,2,1); scatter(x, z, 12.5, value, 'filled');
title('Top View'); xlabel('Left-Right (m)'); ylabel('Forward-Backward (m)');
daspect([1 1 1]); hline(0); vline(0);

subplot(2,2,2); scatter(z, y, 12.5, value, 'filled');
title('Side View'); xlabel('Forward-Backward (m)'); ylabel('Up-Down (m)');
daspect([1 1 1]); hline(0); vline(0);

subplot(2,2,3); scatter(x, y, 12.5, value, 'filled');
title('Side View'); xlabel('Left-Right (m)'); ylabel('Up-Down (m)');
daspect([1 1 1]); hline(0); vline(0);

subplot(2,2,4); scatter3(x,z,y,5,value, 'filled'); view(37.5,30);
title ('3D view'); xlabel('Left-Right (m)'); ylabel('Forward-Backward (m)'); zlabel('Up-Down (m)');
daspect([1 1 1]);
[cmap]=buildcmap('ryg'); 
colormap(cmap);

figure(2);
scatter(x, z, 10, value, 'filled');
title('Top View'); xlabel('Left-Right (m)'); ylabel('Forward-Backward (m)');
daspect([1 1 1]); grid on; colorbar;
[cmap]=buildcmap('ryg'); 
colormap(cmap);

figure(3);
scatter3(x,z,y,10,value, 'filled'); view(110,30);
title ('Mound Bottom - Region 2'); xlabel('Left-Right (m)'); ylabel('Forward-Backward (m)'); zlabel('Up-Down (m)');
daspect([1 1 1]);
[cmap]=buildcmap('ryg'); colormap(cmap); cbar = colorbar; ylabel(cbar, 'NDVI');
%colormap(flipud(summer)); colorbar;
