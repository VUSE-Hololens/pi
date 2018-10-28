% MergeToNDVI
% Script for merging RGB and NDRE images taken by the Sentera into a single
% NDVI image
% Instructions: Fill out the two fields in the Input section below, then
% run. The NDVI image will appear in a popup from which it can be saved as a
% .jpg


% ----- INPUT -----
% session: enter name of session. This will also be the directory name.
session = 'test';
% imgName: enter name of image of interest. It will be the same in both the
% nRGB and NDRE folders.
imgName = 'IMG_00049.jpg';
% ----- End Input: just hit run -----

recvRaw = true;
formatCsharp = false;

rgbImg = [session '/RGB/' imgName];
nirImg = [session '/NIR/' imgName];

rgbRaw = imread(rgbImg);
nirRaw = imread(nirImg);

rgbRaw = imresize(rgbRaw, 1);
nirRaw = imresize(nirRaw, 1);

bandData = zeros(size(rgbRaw,1), size(rgbRaw,2), 5);

if (recvRaw)
    bandData(:,:,1) = -0.061*rgbRaw(:,:,1) - 0.182*rgbRaw(:,:,2) + 1.377*rgbRaw(:,:,3); % blue
    bandData(:,:,2) = -0.329*rgbRaw(:,:,1) + 1.420*rgbRaw(:,:,2) - 0.199*rgbRaw(:,:,3); % green
    bandData(:,:,3) = +1.150*rgbRaw(:,:,1) - 0.110*rgbRaw(:,:,2) - 0.034*rgbRaw(:,:,3); % red
    bandData(:,:,4) = +1.000*nirRaw(:,:,1) - 0.956*nirRaw(:,:,3); % red edge
    bandData(:,:,5) = -0.341*nirRaw(:,:,1) + 2.436*nirRaw(:,:,3); % NIR
else
    bandData(:,:,1) = rgbRaw(:,:,3); % blue
    bandData(:,:,2) = rgbRaw(:,:,2); % green
    bandData(:,:,3) = rgbRaw(:,:,1); % red
    bandData(:,:,4) = nirRaw(:,:,1); % red edge
    bandData(:,:,5) = nirRaw(:,:,3); % NIR
end

ndvi = (2.700*bandData(:,:,5) - bandData(:,:,3)) ./ (2.700*bandData(:,:,5) + bandData(:,:,3));
ndvi(ndvi<0) = 0;
ndre = (bandData(:,:,5) - bandData(:,:,4)) ./ (bandData(:,:,5) + bandData(:,:,4));
ndre(ndre<0) = 0;

colorImg = zeros(size(rgbRaw,1), size(rgbRaw,2), 3);
colorImg(:,:,1) = bandData(:,:,3);
colorImg(:,:,2) = bandData(:,:,2);
colorImg(:,:,3) = bandData(:,:,1);

if (formatCsharp)
    fprintf("public static int height = %d;\n", size(ndvi,1));
    fprintf("public static int width = %d;\n", size(ndvi,2));
    fprintf("public static byte[] NDVI = new byte[] {\n");
    for i = 1:size(ndvi,1)
        fprintf("\t");
        for j = 1:size(ndvi,2)
            fprintf("0x%s", dec2hex(round(ndvi(i,j)*255)));
            if ~((i == size(ndvi,1)) && (j == size(ndvi,2)))
                fprintf(", ");
            end
        end
        fprintf("\n");
    end
    fprintf("};\n");
end

imshow(ndvi);
