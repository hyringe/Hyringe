package hycallparser.parameter;

public class ParameterString extends Parameter {

    private String value;

    public ParameterString(String value) {
        this.value = value;
    }

    @Override
    public boolean isString() {
        return true;
    }

    @Override
    public String getString() {
        return value;
    }

    @Override
    public String toString() {
        return "\"" + value + "\"";
    }
}
