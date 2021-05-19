package hyperv.report;

import hyperv.general.ByteTransformUtils;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

public class ResultFileWalker {

    private BufferedInputStream bis;

    public ResultFileWalker(String resultfile) {
        try {
            bis = new BufferedInputStream(new FileInputStream(resultfile));
        } catch (FileNotFoundException e) {
            throw new IllegalArgumentException("Result file \"" + resultfile + "\" cannot be opened");
        }
    }

    public long read(int size) {
        byte[] buf = new byte[size];
        try {
            if (bis.read(buf) < size)
                throw new IllegalArgumentException("Unexpectedly reached end of file while reading result file");
        } catch (IOException e) {
            throw new IllegalArgumentException("Error while trying to read result file: " + e.getMessage());
        }
        return ByteTransformUtils.bytesToLong(buf);
    }

    public void skip(int size) {
        try {
//            bis.skipNBytes(size);
            bis.skip(size); // why does this not work?
        } catch (IOException e) {
            throw new IllegalArgumentException("Error while trying to read result file: " + e.getMessage());
        }
    }

    public void skipOutputpage() {
        skip(4096);
    }

    public byte[] readOutputpage() {
        try {
            byte[] buf = new byte[4096];
            bis.read(buf);
            return buf;
        } catch (IOException e) {
            throw new IllegalArgumentException("Error while trying to read result file: " + e.getMessage());
        }
    }
}