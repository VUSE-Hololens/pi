% GetBands
% pulls R, G, B bands from rgb image

rgb_filepath = 'mb_rgb.jpg';
nir_filepath = 'mb_nir.jpg';
ndvi_filepath = 'mb_ndvi.jpg';

rgb = imread(rgb_filepath);
nir = imread(nir_filepath);
ndvi = imread(ndvi_filepath);

red_rgb = rgb(:,:,1);
nir_nir = nir(:,:,3);
red_ndvi = ndvi(:,:,1);
nir_ndvi = ndvi(:,:,2);
ndvi_ndvi = ndvi(:,:,3);

figure;
subplot(3,3,1); imshow(rgb); title('RGB');
subplot(3,3,2); imshow(nir); title('NIR');
subplot(3,3,3); imshow(ndvi); title('NDVI');
subplot(3,3,4); imshow(red_rgb); title('RGB: Red');
subplot(3,3,5); imshow(nir_nir); title('NIR: nir');
subplot(3,3,6); imshow(ndvi_ndvi); title('NDVI: NDVI');
subplot(3,3,7); imshow(red_ndvi); title('NDVI: red');
subplot(3,3,8); imshow(nir_ndvi); title('NDVI: nir');

figure;
subplot(1,2,1); imshow(ndvi_ndvi, []); title('NDVI: Pi');
subplot(1,2,2); histogram(ndvi_ndvi);

