function A = rbread(filename, compress)

if (nargin < 1 || nargout > 1 || nargin > 2)
    error('Usage: A = rbread(filename, [compress])');
end

if (nargin < 2)
    compress = 'auto';
end

flag = 0;
[filedir, name, ext] = fileparts(filename);
s = what(filedir);
filename = fullfile(s.path, [name ext]);
switch (compress)
    case 'auto'
        if strcmp(ext, '.gz')
            flag = 1;
        elseif strcmp(ext, '.bz2')
            flag = 2;
        end
    case {'gzip', 'gz'}
        flag = 1;
    case {'bz2', 'bzip2'}
        flag = 2;
    otherwise
        error('Unknown compress mode %s', compress);
end

% call mex file
[A, sym_flag] = mex_srbio_read(filename, flag);
if sym_flag
    A = A + tril(A, -1)';
end
        