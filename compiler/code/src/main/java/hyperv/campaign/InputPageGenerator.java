package hyperv.campaign;

import hyperv.general.ByteTransformUtils;
import org.apache.commons.lang3.ArrayUtils;

import java.util.Arrays;
import java.util.List;

public class InputPageGenerator {

    private byte[] page;

    public InputPageGenerator(int requiredSize) {
        page = new byte[requiredSize+2];
        putValue(requiredSize, -2, 2);
    }

    public void putValue(long value, int offset, int size) {
        byte[] val = ByteTransformUtils.longToBytes(value, size);
        System.arraycopy(val, 0, page, 2 + offset, size);
    }

    public List<Byte> getPage() {
        return Arrays.asList(ArrayUtils.toObject(page));
    }

    public int getSize() {
        return page.length;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        InputPageGenerator that = (InputPageGenerator) o;
        return Arrays.equals(page, that.page);
    }

    @Override
    public int hashCode() {
        return Arrays.hashCode(page);
    }
}
